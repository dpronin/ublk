#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>

#include <algorithm>
#include <memory>
#include <span>
#include <vector>

#include "utils/size_units.hpp"

#include "raid1/target.hpp"

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

struct RAID1Param {
  size_t read_block_per_hs_sz;
  size_t hs_nr;
  size_t hs_storage_sz;
};

using RAID1 = TestWithParam<RAID1Param>;

TEST_P(RAID1, TestReading) {
  auto const &param{GetParam()};

  std::vector<std::shared_ptr<MockRWHandler>> hs{param.hs_nr};
  std::ranges::generate(hs, [] { return std::make_shared<MockRWHandler>(); });

  std::vector<std::unique_ptr<std::byte[]>> storages{hs.size()};
  auto const storage_sz{param.hs_storage_sz};
  std::ranges::generate(storages, [storage_sz] {
    return ut::make_unique_random_bytes(storage_sz);
  });

  auto const reads_nr{div_round_up(storage_sz, param.read_block_per_hs_sz)};

  ublk::raid1::Target tgt{param.read_block_per_hs_sz, {hs.begin(), hs.end()}};
  for (size_t i = 0; i < hs.size(); ++i) {
    EXPECT_CALL(*hs[i], submit(An<std::shared_ptr<read_query>>()))
        .Times(reads_nr / hs.size() + (i < (reads_nr % hs.size())))
        .WillRepeatedly(ut::make_inmem_reader({storages[i].get(), storage_sz}));
  }

  auto const buf_sz{param.hs_storage_sz};
  auto const buf{std::make_unique<std::byte[]>(buf_sz)};
  auto const buf_span{std::span{buf.get(), buf_sz}};

  tgt.process(read_query::create(buf_span, 0));

  for (size_t off = 0; off < buf_span.size();
       off += param.read_block_per_hs_sz) {
    auto const sid = (off / param.read_block_per_hs_sz) % hs.size();
    auto const s1{
        std::as_bytes(buf_span.subspan(off, param.read_block_per_hs_sz))};
    auto const s2{
        std::as_bytes(
            std::span{storages[sid].get() + off, param.read_block_per_hs_sz}),
    };
    EXPECT_TRUE(std::ranges::equal(s1, s2));
  }
}

TEST_P(RAID1, TestWriting) {
  auto const &param{GetParam()};

  std::vector<std::shared_ptr<MockRWHandler>> hs{param.hs_nr};
  std::ranges::generate(hs, [] { return std::make_shared<MockRWHandler>(); });

  std::vector<std::unique_ptr<std::byte[]>> storages{hs.size()};
  auto const storage_sz{param.hs_storage_sz};
  std::ranges::generate(storages, [storage_sz] {
    return std::make_unique<std::byte[]>(storage_sz);
  });

  ublk::raid1::Target tgt{param.read_block_per_hs_sz, {hs.begin(), hs.end()}};
  for (size_t i = 0; i < hs.size(); ++i) {
    EXPECT_CALL(*hs[i], submit(An<std::shared_ptr<write_query>>()))
        .Times(1)
        .WillRepeatedly(ut::make_inmem_writer({storages[i].get(), storage_sz}));
  }

  auto const buf_sz{storage_sz};
  auto const buf{ut::make_unique_random_bytes(buf_sz)};
  auto const buf_span{std::as_bytes(std::span{buf.get(), buf_sz})};

  tgt.process(write_query::create(buf_span, 0));

  for (auto const &storage : storages) {
    auto const s1{buf_span};
    auto const s2{
        std::as_bytes(std::span{storage.get(), storage_sz}),
    };
    EXPECT_TRUE(std::ranges::equal(s1, s2));
  }
}

INSTANTIATE_TEST_SUITE_P(RAID1_Operations, RAID1,
                         Values(
                             RAID1Param{
                                 .read_block_per_hs_sz = 512,
                                 .hs_nr = 4,
                                 .hs_storage_sz = 2_KiB,
                             },
                             RAID1Param{
                                 .read_block_per_hs_sz = 4_KiB,
                                 .hs_nr = 2,
                                 .hs_storage_sz = 40_KiB,
                             },
                             RAID1Param{
                                 .read_block_per_hs_sz = 128_KiB,
                                 .hs_nr = 8,
                                 .hs_storage_sz = 1_MiB,
                             }));
