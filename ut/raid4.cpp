#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>

#include <algorithm>
#include <memory>
#include <random>
#include <span>
#include <vector>

#include "utils/size_units.hpp"

#include "raid4/target.hpp"

#include "read_query.hpp"
#include "rw_handler_interface.hpp"
#include "write_query.hpp"

using namespace ublk;
using namespace testing;

class MockRWHandler : public IRWHandler {
public:
  MOCK_METHOD(int, submit, (std::shared_ptr<read_query>), (noexcept));
  MOCK_METHOD(int, submit, (std::shared_ptr<write_query>), (noexcept));
};

struct RAID4Param {
  size_t strip_sz;
  size_t hs_nr;
  size_t stripes_nr;
};

using RAID4 = TestWithParam<RAID4Param>;

TEST_P(RAID4, TestReading) {
  auto const &param{GetParam()};

  std::vector<std::shared_ptr<MockRWHandler>> hs{param.hs_nr};
  std::ranges::generate(hs, [] { return std::make_shared<MockRWHandler>(); });

  std::vector<std::unique_ptr<std::byte[]>> storages{hs.size() - 1};
  auto const storage_sz{param.strip_sz * param.stripes_nr};
  std::ranges::generate(storages, [storage_sz] {
    auto storage = std::make_unique_for_overwrite<std::byte[]>(storage_sz);
    std::generate_n(storage.get(), storage_sz,
                    [rd = std::random_device{}] mutable {
                      return static_cast<std::byte>(rd());
                    });
    return storage;
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

  ublk::raid4::Target tgt{param.strip_sz, {hs.begin(), hs.end()}};
  for (size_t i = 0; i < hs.size() - 1; ++i) {
    EXPECT_CALL(*hs[i], submit(An<std::shared_ptr<read_query>>()))
        .Times(param.stripes_nr)
        .WillRepeatedly(make_backend_reader(storages[i]));
  }
  EXPECT_CALL(*hs.back(), submit(An<std::shared_ptr<read_query>>())).Times(0);

  auto const buf_sz{(hs.size() - 1) * param.strip_sz * param.stripes_nr};
  auto buf{std::make_unique<std::byte[]>(buf_sz)};
  auto const buf_span{std::span{buf.get(), buf_sz}};

  tgt.process(read_query::create(buf_span, 0));

  for (size_t off = 0; off < buf_span.size(); off += param.strip_sz) {
    auto const sid = (off / param.strip_sz) % (hs.size() - 1);
    auto const soff = off / (param.strip_sz * (hs.size() - 1)) * param.strip_sz;
    auto const s1{std::as_bytes(buf_span.subspan(off, param.strip_sz))};
    auto const s2{
        std::as_bytes(std::span{storages[sid].get() + soff, param.strip_sz}),
    };
    EXPECT_TRUE(std::ranges::equal(s1, s2));
  }
}

TEST_P(RAID4, TestWriting) {
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

  ublk::raid4::Target tgt{param.strip_sz, {hs.begin(), hs.end()}};
  for (size_t i = 0; i < hs.size(); ++i) {
    EXPECT_CALL(*hs[i], submit(An<std::shared_ptr<write_query>>()))
        .Times(param.stripes_nr)
        .WillRepeatedly(make_backend_writer(storages[i]));
  }

  auto const buf_sz{(hs.size() - 1) * param.strip_sz * param.stripes_nr};
  auto buf{std::make_unique_for_overwrite<std::byte[]>(buf_sz)};
  std::generate_n(buf.get(), buf_sz, [rd = std::random_device{}] mutable {
    return static_cast<std::byte>(rd());
  });
  auto const buf_span{std::as_bytes(std::span{buf.get(), buf_sz})};

  tgt.process(write_query::create(buf_span, 0));

  for (size_t off = 0; off < buf_span.size(); off += param.strip_sz) {
    auto const sid = (off / param.strip_sz) % (hs.size() - 1);
    auto const soff = off / (param.strip_sz * (hs.size() - 1)) * param.strip_sz;
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

INSTANTIATE_TEST_SUITE_P(RAID4_Operations, RAID4,
                         Values(
                             RAID4Param{
                                 .strip_sz = 512,
                                 .hs_nr = 3,
                                 .stripes_nr = 4,
                             },
                             RAID4Param{
                                 .strip_sz = 4_KiB,
                                 .hs_nr = 5,
                                 .stripes_nr = 10,
                             },
                             RAID4Param{
                                 .strip_sz = 128_KiB,
                                 .hs_nr = 9,
                                 .stripes_nr = 6,
                             }));
