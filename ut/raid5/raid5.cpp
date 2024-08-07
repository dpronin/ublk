#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>

#include <algorithm>
#include <functional>
#include <memory>
#include <numeric>
#include <ranges>
#include <span>
#include <vector>

#include <range/v3/view/concat.hpp>

#include "mm/mem.hpp"

#include "utils/size_units.hpp"

#include "raid5/target.hpp"

#include "read_query.hpp"
#include "write_query.hpp"

#include "helpers.hpp"

using namespace ublk;
using namespace testing;

namespace {

struct RAID5Param {
  size_t strip_sz;
  size_t hs_nr;
  size_t stripes_nr;
};

using RAID5 = TestWithParam<RAID5Param>;

} // namespace

TEST_P(RAID5, SuccessfulReadingAllStripesAtOnce) {
  auto const &param{GetParam()};

  std::vector<std::shared_ptr<ut::MockRWHandler>> hs{param.hs_nr};
  std::ranges::generate(
      hs, [] { return std::make_shared<StrictMock<ut::MockRWHandler>>(); });

  auto const storage_sz{param.strip_sz * param.stripes_nr};
  auto const storages{
      ut::make_unique_randomized_storages(storage_sz, hs.size()),
  };
  auto const storage_spans{ut::storages_to_const_spans(storages, storage_sz)};

  auto const stripe_data_sz{(hs.size() - 1) * param.strip_sz};

  auto target{ublk::raid5::Target{param.strip_sz, {hs.begin(), hs.end()}}};

  auto const parity_full_cycles{param.stripes_nr / hs.size()};
  auto const parity_cycle_rem{param.stripes_nr % hs.size()};

  for (auto i : std::views::iota(0uz, hs.size())) {
    EXPECT_CALL(*hs[i], submit(Matcher<std::shared_ptr<read_query>>(NotNull())))
        .Times((hs.size() - 1) * parity_full_cycles + parity_cycle_rem -
               !(i < (hs.size() - parity_cycle_rem)))
        .WillRepeatedly(ut::make_inmem_reader(storage_spans[i]));
  }

  auto const buf_sz{stripe_data_sz * param.stripes_nr};
  auto const buf{mm::make_unique_zeroed_bytes(buf_sz)};
  auto const buf_span{std::span{buf.get(), buf_sz}};

  target.process(read_query::create(buf_span, 0));

  for (auto off{0uz}; off < buf_span.size(); off += param.strip_sz) {
    auto const stripe_id{off / stripe_data_sz};
    auto const sid_parity{hs.size() - (stripe_id % hs.size()) - 1};
    auto const hs_first_part{std::views::iota(0uz, sid_parity)};
    auto const hs_last_part{std::views::iota(sid_parity + 1, hs.size())};
    auto const sids{ranges::views::concat(hs_first_part, hs_last_part)};
    auto const sid{sids[(off / param.strip_sz) % (hs.size() - 1)]};
    auto const soff{stripe_id * param.strip_sz};
    auto const s1{std::as_bytes(buf_span.subspan(off, param.strip_sz))};
    auto const s2{storage_spans[sid].subspan(soff, param.strip_sz)};
    EXPECT_THAT(s1, ElementsAreArray(s2));
  }
}

TEST_P(RAID5, SuccessfulWritingAllStripesAtOnceTwice) {
  auto const &param{GetParam()};

  std::vector<std::shared_ptr<ut::MockRWHandler>> hs{param.hs_nr};
  std::ranges::generate(
      hs, [] { return std::make_shared<StrictMock<ut::MockRWHandler>>(); });

  auto const storage_sz{param.strip_sz * param.stripes_nr};
  auto const storages{
      ut::make_unique_zeroed_storages(storage_sz, hs.size()),
  };
  auto const storage_spans{ut::storages_to_spans(storages, storage_sz)};

  auto target{ublk::raid5::Target{param.strip_sz, {hs.begin(), hs.end()}}};

  for (auto const stripe_data_sz{(hs.size() - 1) * param.strip_sz};
       auto const i [[maybe_unused]] : std::views::iota(0uz, 2uz)) {
    /* clang-format off */
    for (auto const &[h, storage_span] : std::views::zip(std::views::all(hs), storage_spans)) {
      EXPECT_CALL(*h, submit(Matcher<std::shared_ptr<write_query>>(NotNull())))
          .Times(param.stripes_nr)
          .WillRepeatedly(ut::make_inmem_writer(storage_span));
    }
    /* clang-format on */

    auto const buf_sz{stripe_data_sz * param.stripes_nr};
    auto const buf{mm::make_unique_randomized_bytes(buf_sz)};
    auto const buf_span{std::as_bytes(std::span{buf.get(), buf_sz})};

    target.process(write_query::create(buf_span, 0));

    for (auto off{0uz}; off < buf_span.size(); off += param.strip_sz) {
      auto const stripe_id{off / stripe_data_sz};
      auto const sid_parity{hs.size() - (stripe_id % hs.size()) - 1};
      auto const hs_first_part{std::views::iota(0uz, sid_parity)};
      auto const hs_last_part{std::views::iota(sid_parity + 1, hs.size())};
      auto const sids{ranges::views::concat(hs_first_part, hs_last_part)};
      auto const sid{sids[(off / param.strip_sz) % (hs.size() - 1)]};
      auto const soff{stripe_id * param.strip_sz};
      auto const s1{buf_span.subspan(off, param.strip_sz)};
      auto const s2{
          std::as_bytes(storage_spans[sid].subspan(soff, param.strip_sz)),
      };
      EXPECT_THAT(s1, ElementsAreArray(s2));
    }

    ut::parity_verify(storage_spans, param.strip_sz);
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
