#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>

#include <algorithm>
#include <memory>
#include <ranges>
#include <span>
#include <vector>

#include "mm/mem.hpp"

#include "utils/size_units.hpp"

#include "raid1/target.hpp"

#include "read_query.hpp"
#include "write_query.hpp"

#include "helpers.hpp"

using namespace ublk;
using namespace testing;

struct RAID1Param {
  size_t read_block_per_hs_sz;
  size_t hs_nr;
  size_t hs_storage_sz;
};

using RAID1 = TestWithParam<RAID1Param>;

TEST_P(RAID1, TestReading) {
  auto const &param{GetParam()};

  std::vector<std::shared_ptr<ut::MockRWHandler>> hs{param.hs_nr};
  std::ranges::generate(
      hs, [] { return std::make_shared<StrictMock<ut::MockRWHandler>>(); });

  auto const storages{
      ut::make_unique_randomized_storages(param.hs_storage_sz, hs.size()),
  };

  auto const storage_spans{
      ut::storages_to_const_spans(storages, param.hs_storage_sz),
  };

  auto const reads_nr{
      div_round_up(param.hs_storage_sz, param.read_block_per_hs_sz),
  };

  auto tgt{
      ublk::raid1::Target{param.read_block_per_hs_sz, {hs.begin(), hs.end()}},
  };

  for (auto i : std::views::iota(0uz, hs.size())) {
    EXPECT_CALL(*hs[i], submit(An<std::shared_ptr<read_query>>()))
        .Times(reads_nr / hs.size() + (i < (reads_nr % hs.size())))
        .WillRepeatedly(ut::make_inmem_reader(storage_spans[i]));
  }

  auto const buf_sz{param.hs_storage_sz};
  auto const buf{std::make_unique<std::byte[]>(buf_sz)};
  auto const buf_span{std::span{buf.get(), buf_sz}};

  tgt.process(read_query::create(buf_span, 0));

  for (size_t off = 0; off < buf_span.size();
       off += param.read_block_per_hs_sz) {
    auto const sid{(off / param.read_block_per_hs_sz) % hs.size()};
    auto const s1{
        std::as_bytes(buf_span.subspan(off, param.read_block_per_hs_sz)),
    };
    auto const s2{storage_spans[sid].subspan(off, param.read_block_per_hs_sz)};
    EXPECT_THAT(s1, ElementsAreArray(s2));
  }
}

TEST_P(RAID1, TestWriting) {
  auto const &param{GetParam()};

  std::vector<std::shared_ptr<ut::MockRWHandler>> hs{param.hs_nr};
  std::ranges::generate(hs,
                        [] { return std::make_shared<ut::MockRWHandler>(); });

  auto const storages{
      ut::make_unique_zeroed_storages(param.hs_storage_sz, hs.size()),
  };
  auto const storage_spans{
      ut::storages_to_spans(storages, param.hs_storage_sz),
  };

  auto tgt{
      ublk::raid1::Target{param.read_block_per_hs_sz, {hs.begin(), hs.end()}},
  };
  /* clang-format off */
  for (auto const &[h, storage_span] : std::views::zip(std::views::all(hs), storage_spans)) {
    EXPECT_CALL(*h, submit(An<std::shared_ptr<write_query>>()))
        .Times(1)
        .WillRepeatedly(ut::make_inmem_writer(storage_span));
  }
  /* clang-format on */

  auto const buf_sz{param.hs_storage_sz};
  auto const buf{mm::make_unique_randomized_bytes(buf_sz)};
  auto const buf_span{std::as_bytes(std::span{buf.get(), buf_sz})};

  tgt.process(write_query::create(buf_span, 0));

  /* clang-format off */
  for (auto storage_span :   storage_spans
                           | std::views::transform([](auto s) { return std::as_bytes(s); })) {
    auto const s1{buf_span};
    auto const s2{storage_span};
    EXPECT_THAT(s1, ElementsAreArray(s2));
  }
  /* clang-format on */
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
