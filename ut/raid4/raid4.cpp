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

#include "mm/mem.hpp"

#include "utils/size_units.hpp"

#include "raid4/target.hpp"

#include "read_query.hpp"
#include "write_query.hpp"

#include "helpers.hpp"

using namespace ublk;
using namespace testing;

namespace {

struct RAID4Param {
  size_t strip_sz;
  size_t hs_nr;
  size_t stripes_nr;
};

using RAID4 = TestWithParam<RAID4Param>;

template <is_byte T>
void parity_verify(std::vector<std::span<T>> const &storage_spans,
                   size_t check_sz) noexcept {
  for (auto i : std::views::iota(0uz, check_sz)) {
    EXPECT_EQ(std::reduce(storage_spans.begin(), storage_spans.end() - 1,
                          storage_spans.end()[-1][i],
                          [i, op = std::bit_xor<>{}](auto &&arg1, auto &&arg2) {
                            using T1 = std::decay_t<decltype(arg1)>;
                            using T2 = std::decay_t<decltype(arg2)>;
                            if constexpr (std::same_as<T1, std::byte>) {
                              if constexpr (std::same_as<T2, std::byte>) {
                                return op(arg1, arg2);
                              } else {
                                return op(arg1, arg2[i]);
                              }
                            } else {
                              if constexpr (std::same_as<T2, std::byte>) {
                                return op(arg1[i], arg2);
                              } else {
                                return op(arg1[i], arg2[i]);
                              }
                            }
                          }),
              0_b);
  }
}

} // namespace

TEST_P(RAID4, SuccessfulReadingAllStripesAtOnce) {
  auto const &param{GetParam()};

  std::vector<std::shared_ptr<ut::MockRWHandler>> hs{param.hs_nr};
  std::ranges::generate(
      hs, [] { return std::make_shared<StrictMock<ut::MockRWHandler>>(); });

  auto const storage_sz{param.strip_sz * param.stripes_nr};
  auto const storages{
      ut::make_unique_randomized_storages(storage_sz, hs.size() - 1),
  };
  auto const storage_spans{ut::storages_to_const_spans(storages, storage_sz)};

  auto const stripe_data_sz{(hs.size() - 1) * param.strip_sz};

  auto target{ublk::raid4::Target{param.strip_sz, {hs.begin(), hs.end()}}};

  /* clang-format off */
  for (auto const &[h, storage_span] :   std::views::zip(std::views::all(hs), storage_spans)
                                       | std::views::take(hs.size() - 1)) {
    EXPECT_CALL(*h, submit(An<std::shared_ptr<read_query>>()))
        .Times(param.stripes_nr)
        .WillRepeatedly(ut::make_inmem_reader(storage_span));
  }
  /* clang-format on */
  EXPECT_CALL(*hs.back(), submit(An<std::shared_ptr<read_query>>())).Times(0);

  auto const buf_sz{stripe_data_sz * param.stripes_nr};
  auto const buf{mm::make_unique_zeroed_bytes(buf_sz)};
  auto const buf_span{std::span{buf.get(), buf_sz}};

  target.process(read_query::create(buf_span, 0));

  for (auto off{0uz}; off < buf_span.size(); off += param.strip_sz) {
    auto const sid{(off / param.strip_sz) % (hs.size() - 1)};
    auto const soff{off / stripe_data_sz * param.strip_sz};
    auto const s1{std::as_bytes(buf_span.subspan(off, param.strip_sz))};
    auto const s2{storage_spans[sid].subspan(soff, param.strip_sz)};
    EXPECT_THAT(s1, ElementsAreArray(s2));
  }
}

TEST_P(RAID4, SuccessfulWritingAllStripesAtOnceTwice) {
  auto const &param{GetParam()};

  std::vector<std::shared_ptr<ut::MockRWHandler>> hs{param.hs_nr};
  std::ranges::generate(hs,
                        [] { return std::make_shared<ut::MockRWHandler>(); });

  auto const storage_sz{param.strip_sz * param.stripes_nr};
  auto const storages{
      ut::make_unique_zeroed_storages(storage_sz, hs.size()),
  };
  auto const storage_spans{ut::storages_to_spans(storages, storage_sz)};

  auto target{ublk::raid4::Target{param.strip_sz, {hs.begin(), hs.end()}}};

  for (auto const stripe_data_sz{(hs.size() - 1) * param.strip_sz};
       auto const i [[maybe_unused]] : std::views::iota(0uz, 2uz)) {
    /* clang-format off */
    for (auto const &[h, storage_span] : std::views::zip(std::views::all(hs), storage_spans)) {
      EXPECT_CALL(*h, submit(An<std::shared_ptr<write_query>>()))
          .Times(param.stripes_nr)
          .WillRepeatedly(ut::make_inmem_writer(storage_span));
    }
    /* clang-format on */

    auto const buf_sz{stripe_data_sz * param.stripes_nr};
    auto const buf{mm::make_unique_randomized_bytes(buf_sz)};
    auto const buf_span{std::as_bytes(std::span{buf.get(), buf_sz})};

    target.process(write_query::create(buf_span, 0));

    for (auto off{0uz}; off < buf_span.size(); off += param.strip_sz) {
      auto const sid{(off / param.strip_sz) % (hs.size() - 1)};
      auto const soff{off / stripe_data_sz * param.strip_sz};
      auto const s1{buf_span.subspan(off, param.strip_sz)};
      auto const s2{
          std::as_bytes(storage_spans[sid].subspan(soff, param.strip_sz)),
      };
      EXPECT_THAT(s1, ElementsAreArray(s2));
    }

    parity_verify(storage_spans, param.strip_sz);
  }
}

TEST_P(RAID4, SuccessfulWritingFullThenPartialStripesWriting) {
  auto const &param{GetParam()};

  std::vector<std::shared_ptr<ut::MockRWHandler>> hs{param.hs_nr};
  std::ranges::generate(hs,
                        [] { return std::make_shared<ut::MockRWHandler>(); });

  auto const hs_data{std::views::all(hs) | std::views::take(hs.size() - 1)};

  auto const storage_sz{param.strip_sz * param.stripes_nr};
  auto const storages{
      ut::make_unique_zeroed_storages(storage_sz, hs.size()),
  };
  auto const storage_spans{ut::storages_to_spans(storages, storage_sz)};

  auto const storage_spans_data{
      std::views::all(storage_spans) |
          std::views::take(storage_spans.size() - 1),
  };

  auto target{ublk::raid4::Target{param.strip_sz, {hs.begin(), hs.end()}}};

  auto const stripe_data_sz{(hs.size() - 1) * param.strip_sz};

  for (auto const stripe_id : std::views::iota(0uz, param.stripes_nr)) {
    /* clang-format off */
    for (auto const &[h, storage_span] : std::views::zip(std::views::all(hs), storage_spans)) {
      EXPECT_CALL(*h, submit(An<std::shared_ptr<write_query>>()))
          .WillOnce(ut::make_inmem_writer(storage_span));
    }
    /* clang-format on */

    auto const buf{mm::make_unique_randomized_bytes(stripe_data_sz)};
    auto const buf_span{std::span<std::byte const>{buf.get(), stripe_data_sz}};

    auto const start_off{stripe_id * stripe_data_sz};

    target.process(write_query::create(buf_span, start_off));

    for (auto off{start_off}; off < start_off + buf_span.size();
         off += param.strip_sz) {
      auto const sid{(off / param.strip_sz) % (hs.size() - 1)};
      auto const soff{off / stripe_data_sz * param.strip_sz};
      auto const s1{buf_span.subspan(off - start_off, param.strip_sz)};
      auto const s2{
          std::as_bytes(storage_spans[sid].subspan(soff, param.strip_sz)),
      };
      ASSERT_THAT(s1, ElementsAreArray(s2));
    }
  }

  parity_verify(storage_spans, param.strip_sz);

  for (auto const write_sz{stripe_data_sz / 2};
       auto const stripe_id : std::views::iota(0uz, param.stripes_nr)) {

    for (auto const &[h, storage_span] :
         std::views::zip(hs_data, storage_spans_data) |
             std::views::take(div_round_up(write_sz, param.strip_sz))) {
      EXPECT_CALL(*h, submit(An<std::shared_ptr<read_query>>()))
          .WillOnce(ut::make_inmem_reader(storage_span));
      EXPECT_CALL(*h, submit(An<std::shared_ptr<write_query>>()))
          .WillOnce(ut::make_inmem_writer(storage_span));
    }

    EXPECT_CALL(*hs.back(), submit(An<std::shared_ptr<read_query>>()))
        .WillOnce(ut::make_inmem_reader(storage_spans.back()));
    EXPECT_CALL(*hs.back(), submit(An<std::shared_ptr<write_query>>()))
        .WillOnce(ut::make_inmem_writer(storage_spans.back()));

    auto const buf{mm::make_unique_randomized_bytes(write_sz)};
    auto const buf_span{std::span<std::byte const>{buf.get(), write_sz}};

    auto const start_off{stripe_id * stripe_data_sz};

    target.process(write_query::create(buf_span, start_off));

    for (auto off{start_off}; off + start_off < buf_span.size();) {
      auto const chunk_sz{std::min(param.strip_sz, buf_span.size() - off)};
      auto const sid{(off / param.strip_sz) % (hs.size() - 1)};
      auto const soff{off / stripe_data_sz * param.strip_sz};
      auto const s1{buf_span.subspan(off - start_off, chunk_sz)};
      auto const s2{
          std::as_bytes(storage_spans[sid].subspan(soff, chunk_sz)),
      };
      EXPECT_THAT(s1, ElementsAreArray(s2));
      off += chunk_sz;
    }
  }

  parity_verify(storage_spans, param.strip_sz);
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
