#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>

#include <algorithm>
#include <memory>
#include <span>
#include <vector>

#include "utils/size_units.hpp"

#include "raid0/target.hpp"

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

struct RAID0Param {
  size_t strip_sz;
  size_t hs_nr;
  size_t stripes_nr;
};

using RAID0 = TestWithParam<RAID0Param>;

TEST_P(RAID0, TestReading) {
  auto const &param{GetParam()};

  std::vector<std::shared_ptr<MockRWHandler>> hs{param.hs_nr};
  std::ranges::generate(hs, [] { return std::make_shared<MockRWHandler>(); });

  std::vector<std::unique_ptr<std::byte[]>> storages{hs.size()};
  auto const storage_sz = param.strip_sz * param.stripes_nr;
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

  ublk::raid0::Target tgt{param.strip_sz, {hs.begin(), hs.end()}};
  for (size_t i = 0; i < hs.size(); ++i) {
    EXPECT_CALL(*hs[i], submit(An<std::shared_ptr<read_query>>()))
        .Times(param.stripes_nr)
        .WillRepeatedly(make_backend_reader(storages[i]));
  }

  auto const buf_sz{hs.size() * param.strip_sz * param.stripes_nr};
  auto const buf{std::make_unique<std::byte[]>(buf_sz)};
  auto const buf_span{std::span{buf.get(), buf_sz}};

  tgt.process(read_query::create(buf_span, 0));

  for (size_t off = 0; off < buf_span.size(); off += param.strip_sz) {
    auto const sid = (off / param.strip_sz) % hs.size();
    auto const soff = off / (param.strip_sz * hs.size()) * param.strip_sz;
    auto const s1{std::as_bytes(buf_span.subspan(off, param.strip_sz))};
    auto const s2{
        std::as_bytes(std::span{storages[sid].get() + soff, param.strip_sz}),
    };
    EXPECT_TRUE(std::ranges::equal(s1, s2));
  }
}

TEST_P(RAID0, TestWriting) {
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

  ublk::raid0::Target tgt{param.strip_sz, {hs.begin(), hs.end()}};
  for (size_t i = 0; i < hs.size(); ++i) {
    EXPECT_CALL(*hs[i], submit(An<std::shared_ptr<write_query>>()))
        .Times(param.stripes_nr)
        .WillRepeatedly(make_backend_writer(storages[i]));
  }

  auto const buf_sz{hs.size() * param.strip_sz * param.stripes_nr};
  auto const buf{ut::make_unique_random_bytes(buf_sz)};
  auto const buf_span = std::as_bytes(std::span{buf.get(), buf_sz});

  tgt.process(write_query::create(buf_span, 0));

  for (size_t off = 0; off < buf_span.size(); off += param.strip_sz) {
    auto const sid = (off / param.strip_sz) % hs.size();
    auto const soff = off / (param.strip_sz * hs.size()) * param.strip_sz;
    auto const s1{buf_span.subspan(off, param.strip_sz)};
    auto const s2{
        std::as_bytes(std::span{storages[sid].get() + soff, param.strip_sz}),
    };
    EXPECT_TRUE(std::ranges::equal(s1, s2));
  }
}

INSTANTIATE_TEST_SUITE_P(RAID0_Operations, RAID0,
                         Values(
                             RAID0Param{
                                 .strip_sz = 512,
                                 .hs_nr = 2,
                                 .stripes_nr = 4,
                             },
                             RAID0Param{
                                 .strip_sz = 4_KiB,
                                 .hs_nr = 1,
                                 .stripes_nr = 10,
                             },
                             RAID0Param{
                                 .strip_sz = 128_KiB,
                                 .hs_nr = 8,
                                 .stripes_nr = 6,
                             }));
