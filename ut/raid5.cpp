#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>

#include <algorithm>
#include <memory>
#include <span>
#include <vector>

#include "utils/size_units.hpp"

#include "raid5/target.hpp"

#include "read_query.hpp"
#include "rw_handler_interface.hpp"
#include "write_query.hpp"

#include "helpers.hpp"

using namespace ublk;
using namespace testing;

class MockRWHandler : public IRWHandler {
public:
  MOCK_METHOD(int, submit, (std::shared_ptr<read_query>), (noexcept));
  MOCK_METHOD(int, submit, (std::shared_ptr<write_query>), (noexcept));
};

struct RAID5Param {
  size_t strip_sz;
  size_t hs_nr;
  size_t stripes_nr;
};

using RAID5 = TestWithParam<RAID5Param>;

TEST_P(RAID5, TestReading) {
  auto const &param{GetParam()};

  std::vector<std::shared_ptr<MockRWHandler>> hs{param.hs_nr};
  std::ranges::generate(hs, [] { return std::make_shared<MockRWHandler>(); });

  std::vector<std::unique_ptr<std::byte[]>> storages{hs.size()};
  auto const storage_sz{param.strip_sz * param.stripes_nr};
  std::ranges::generate(storages, [storage_sz] {
    return ut::make_unique_random_bytes(storage_sz);
  });

  auto make_backend_reader = [storage_sz](auto const &storage) {
    return [&, storage_sz](std::shared_ptr<read_query> rq) {
      EXPECT_FALSE(rq->buf().empty());
      EXPECT_LE(rq->buf().size() + rq->offset(), storage_sz);
      std::copy_n(storage.get() + rq->offset(), rq->buf().size(),
                  rq->buf().begin());
      return 0;
    };
  };

  ublk::raid5::Target tgt{param.strip_sz, {hs.begin(), hs.end()}};

  auto const parity_full_cycles{param.stripes_nr / hs.size()};
  auto const parity_cycle_rem{param.stripes_nr % hs.size()};

  for (size_t i = 0; i < hs.size(); ++i) {
    EXPECT_CALL(*hs[i], submit(An<std::shared_ptr<read_query>>()))
        .Times((hs.size() - 1) * parity_full_cycles + parity_cycle_rem -
               !(i < (hs.size() - parity_cycle_rem)))
        .WillRepeatedly(make_backend_reader(storages[i]));
  }

  auto const buf_sz{(hs.size() - 1) * param.strip_sz * param.stripes_nr};
  auto const buf{std::make_unique<std::byte[]>(buf_sz)};
  auto const buf_span{std::span{buf.get(), buf_sz}};

  tgt.process(read_query::create(buf_span, 0));

  std::vector<size_t> sids(hs.size());
  std::iota(sids.begin(), sids.end(), 0);

  for (size_t off = 0; off < buf_span.size(); off += param.strip_sz) {
    auto const stripe_id = off / (param.strip_sz * (hs.size() - 1));
    auto const sid_parity = hs.size() - (stripe_id % hs.size()) - 1;
    std::vector<size_t> sids_rotated(sids.size());
    std::copy_n(sids.begin(), sid_parity, sids_rotated.begin());
    std::rotate_copy(sids.begin() + sid_parity, sids.begin() + sid_parity + 1,
                     sids.end(), sids_rotated.begin() + sid_parity);
    auto const sid = sids_rotated[(off / param.strip_sz) % (hs.size() - 1)];
    auto const soff = stripe_id * param.strip_sz;
    auto const s1{std::as_bytes(buf_span.subspan(off, param.strip_sz))};
    auto const s2{
        std::as_bytes(std::span{storages[sid].get() + soff, param.strip_sz}),
    };
    EXPECT_TRUE(std::ranges::equal(s1, s2));
  }
}

TEST_P(RAID5, TestWriting) {
  auto const &param{GetParam()};

  std::vector<std::shared_ptr<MockRWHandler>> hs{param.hs_nr};
  std::ranges::generate(hs, [] { return std::make_shared<MockRWHandler>(); });

  std::vector<std::unique_ptr<std::byte[]>> storages{hs.size()};
  auto const storage_sz{param.strip_sz * param.stripes_nr};
  std::ranges::generate(storages, [storage_sz] {
    return std::make_unique<std::byte[]>(storage_sz);
  });

  auto make_backend_writer = [storage_sz](auto const &storage) {
    return [&, storage_sz](std::shared_ptr<write_query> wq) {
      EXPECT_FALSE(wq->buf().empty());
      EXPECT_LE(wq->buf().size() + wq->offset(), storage_sz);
      std::ranges::copy(wq->buf(), storage.get() + wq->offset());
      return 0;
    };
  };

  ublk::raid5::Target tgt{param.strip_sz, {hs.begin(), hs.end()}};
  for (size_t i = 0; i < hs.size(); ++i) {
    EXPECT_CALL(*hs[i], submit(An<std::shared_ptr<write_query>>()))
        .Times(param.stripes_nr)
        .WillRepeatedly(make_backend_writer(storages[i]));
  }

  auto const buf_sz{(hs.size() - 1) * param.strip_sz * param.stripes_nr};
  auto const buf{ut::make_unique_random_bytes(buf_sz)};
  auto const buf_span{std::as_bytes(std::span{buf.get(), buf_sz})};

  tgt.process(write_query::create(buf_span, 0));

  std::vector<size_t> sids(hs.size());
  std::iota(sids.begin(), sids.end(), 0);

  for (size_t off = 0; off < buf_span.size(); off += param.strip_sz) {
    auto const stripe_id = off / (param.strip_sz * (hs.size() - 1));
    auto const sid_parity = hs.size() - (stripe_id % hs.size()) - 1;
    std::vector<size_t> sids_rotated(sids.size());
    std::copy_n(sids.begin(), sid_parity, sids_rotated.begin());
    std::rotate_copy(sids.begin() + sid_parity, sids.begin() + sid_parity + 1,
                     sids.end(), sids_rotated.begin() + sid_parity);
    auto const sid = sids_rotated[(off / param.strip_sz) % (hs.size() - 1)];
    auto const soff = stripe_id * param.strip_sz;
    auto const s1{buf_span.subspan(off, param.strip_sz)};
    auto const s2{
        std::as_bytes(std::span{storages[sid].get() + soff, param.strip_sz}),
    };
    EXPECT_TRUE(std::ranges::equal(s1, s2));
  }

  for (size_t i = 0; i < param.strip_sz; ++i) {
    EXPECT_EQ(std::reduce(storages.begin(),
                          storages.begin() + storages.size() - 1,
                          storages.back()[i],
                          [i, op = std::bit_xor<>{}](auto &&arg1, auto &&arg2) {
                            using T1 = std::decay_t<decltype(arg1)>;
                            using T2 = std::decay_t<decltype(arg2)>;
                            if constexpr (std::is_same_v<T1, std::byte>) {
                              if constexpr (std::is_same_v<T2, std::byte>) {
                                return op(arg1, arg2);
                              } else {
                                return op(arg1, arg2[i]);
                              }
                            } else {
                              if constexpr (std::is_same_v<T2, std::byte>) {
                                return op(arg1[i], arg2);
                              } else {
                                return op(arg1[i], arg2[i]);
                              }
                            }
                          }),
              0_b);
  }
}

INSTANTIATE_TEST_SUITE_P(RAID5_Operations, RAID5,
                         Values(
                             RAID5Param{
                                 .strip_sz = 512,
                                 .hs_nr = 3,
                                 .stripes_nr = 4,
                             },
                             RAID5Param{
                                 .strip_sz = 4_KiB,
                                 .hs_nr = 5,
                                 .stripes_nr = 10,
                             },
                             RAID5Param{
                                 .strip_sz = 128_KiB,
                                 .hs_nr = 9,
                                 .stripes_nr = 6,
                             }));