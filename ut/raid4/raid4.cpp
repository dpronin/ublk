#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>

#include <algorithm>
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

struct RAID4Param {
  size_t strip_sz;
  size_t hs_nr;
  size_t stripes_nr;
};

using RAID4 = TestWithParam<RAID4Param>;

TEST_P(RAID4, TestReading) {
  auto const &param{GetParam()};

  std::vector<std::shared_ptr<ut::MockRWHandler>> hs{param.hs_nr};
  std::ranges::generate(
      hs, [] { return std::make_shared<StrictMock<ut::MockRWHandler>>(); });

  auto const storage_sz{param.strip_sz * param.stripes_nr};
  auto const storages{
      ut::make_randomized_unique_storages<std::byte const>(storage_sz,
                                                           hs.size() - 1),
  };
  auto const storage_spans{ut::storages_to_spans(storages, storage_sz)};

  auto tgt{ublk::raid4::Target{param.strip_sz, {hs.begin(), hs.end()}}};
  /* clang-format off */
  for (auto const &[h, storage_span] :   std::views::zip(std::views::all(hs), storage_spans)
                                       | std::views::take(hs.size() - 1)) {
    EXPECT_CALL(*h, submit(An<std::shared_ptr<read_query>>()))
        .Times(param.stripes_nr)
        .WillRepeatedly(ut::make_inmem_reader(storage_span));
  }
  /* clang-format on */
  EXPECT_CALL(*hs.back(), submit(An<std::shared_ptr<read_query>>())).Times(0);

  auto const buf_sz{(hs.size() - 1) * param.strip_sz * param.stripes_nr};
  auto const buf{std::make_unique<std::byte[]>(buf_sz)};
  auto const buf_span{std::span{buf.get(), buf_sz}};

  tgt.process(read_query::create(buf_span, 0));

  for (size_t off = 0; off < buf_span.size(); off += param.strip_sz) {
    auto const sid{(off / param.strip_sz) % (hs.size() - 1)};
    auto const soff{off / (param.strip_sz * (hs.size() - 1)) * param.strip_sz};
    auto const s1{std::as_bytes(buf_span.subspan(off, param.strip_sz))};
    auto const s2{storage_spans[sid].subspan(soff, param.strip_sz)};
    EXPECT_THAT(s1, ElementsAreArray(s2));
  }
}

TEST_P(RAID4, TestWritingAllStripesAtOnce) {
  auto const &param{GetParam()};

  std::vector<std::shared_ptr<ut::MockRWHandler>> hs{param.hs_nr};
  std::ranges::generate(hs,
                        [] { return std::make_shared<ut::MockRWHandler>(); });

  auto const storage_sz{param.strip_sz * param.stripes_nr};
  auto const storages{
      ut::make_unique_zeroed_storages<std::byte>(storage_sz, hs.size()),
  };
  auto const storage_spans{ut::storages_to_spans(storages, storage_sz)};

  auto tgt{ublk::raid4::Target{param.strip_sz, {hs.begin(), hs.end()}}};

  for (auto i [[maybe_unused]] : std::views::iota(0uz, 2uz)) {
    /* clang-format off */
    for (auto const &[h, storage_span] : std::views::zip(std::views::all(hs), storage_spans)) {
      EXPECT_CALL(*h, submit(An<std::shared_ptr<write_query>>()))
          .Times(param.stripes_nr)
          .WillRepeatedly(ut::make_inmem_writer(storage_span));
    }
    /* clang-format on */

    auto const buf_sz{(hs.size() - 1) * param.strip_sz * param.stripes_nr};
    auto const buf{mm::make_unique_randomized_bytes(buf_sz)};
    auto const buf_span{std::as_bytes(std::span{buf.get(), buf_sz})};

    tgt.process(write_query::create(buf_span, 0));

    for (size_t off = 0; off < buf_span.size(); off += param.strip_sz) {
      auto const sid{(off / param.strip_sz) % (hs.size() - 1)};
      auto const soff{off / (param.strip_sz * (hs.size() - 1)) *
                      param.strip_sz};
      auto const s1{buf_span.subspan(off, param.strip_sz)};
      auto const s2{
          std::as_bytes(storage_spans[sid].subspan(soff, param.strip_sz)),
      };
      EXPECT_THAT(s1, ElementsAreArray(s2));
    }

    for (auto i : std::views::iota(0uz, param.strip_sz)) {
      EXPECT_EQ(
          std::reduce(storage_spans.begin(), storage_spans.end() - 1,
                      storage_spans.end()[-1][i],
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
