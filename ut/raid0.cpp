#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>

#include <algorithm>
#include <memory>
#include <ranges>
#include <span>
#include <vector>

#include "utils/size_units.hpp"

#include "raid0/target.hpp"

#include "read_query.hpp"
#include "write_query.hpp"

#include "helpers.hpp"

using namespace ublk;
using namespace testing;

struct RAID0Param {
  size_t strip_sz;
  size_t strips_per_stripe_nr;
  size_t stripes_nr;
};

class RAID0 : public TestWithParam<RAID0Param> {
protected:
  void SetUp() override {
    auto const &param{GetParam()};

    backend_ctxs_.resize(param.strips_per_stripe_nr);
    std::ranges::generate(backend_ctxs_, [] -> backend_ctx {
      return {
          .h = std::make_shared<ut::MockRWHandler>(),
      };
    });

    auto hs =
        std::views::all(backend_ctxs_) |
        std::views::transform([](auto const &hs_ctx) { return hs_ctx.h; });

    target_ = std::make_unique<raid0::Target>(param.strip_sz, hs);
  }

  void TearDown() override {
    target_.reset();
    backend_ctxs_.clear();
  }

  struct backend_ctx {
    std::shared_ptr<ut::MockRWHandler> h;
  };
  std::vector<backend_ctx> backend_ctxs_;

  std::unique_ptr<raid0::Target> target_;
};

TEST_P(RAID0, FullStripeReading) {
  auto const &param{GetParam()};

  auto const storage_sz{param.strip_sz * param.stripes_nr};
  auto const storages{
      ut::make_randomized_storages<std::byte const>(storage_sz,
                                                    backend_ctxs_.size()),
  };
  auto const storage_spans{ut::make_storage_spans(storages, storage_sz)};

  auto const hs =
      std::views::all(backend_ctxs_) |
      std::views::transform([](auto const &hs_ctx) { return hs_ctx.h; });

  auto const buf_sz{
      param.strips_per_stripe_nr * param.strip_sz * param.stripes_nr,
  };
  auto const buf{std::make_unique<std::byte[]>(buf_sz)};
  auto const buf_span{std::span{buf.get(), buf_sz}};

  auto const stripe_sz{param.strip_sz * param.strips_per_stripe_nr};

  for (size_t off{0}; off < buf_span.size(); off += stripe_sz) {
    for (auto const &[h, storage_span] : std::views::zip(hs, storage_spans)) {
      EXPECT_CALL(*h, submit(An<std::shared_ptr<read_query>>()))
          .WillOnce(ut::make_inmem_reader(storage_span));
    }
    target_->process(read_query::create(
        buf_span.subspan(off, stripe_sz), off,
        [](read_query const &rq) { EXPECT_EQ(rq.err(), 0); }));
  }

  for (size_t off = 0; off < buf_span.size(); off += param.strip_sz) {
    auto const sid{(off / param.strip_sz) % param.strips_per_stripe_nr};
    auto const soff{
        off / (param.strip_sz * param.strips_per_stripe_nr) * param.strip_sz,
    };
    auto const s1{std::as_bytes(buf_span.subspan(off, param.strip_sz))};
    auto const s2{storage_spans[sid].subspan(soff, param.strip_sz)};
    EXPECT_THAT(s1, ElementsAreArray(s2));
  }
}

TEST_P(RAID0, FullStripeWriting) {
  auto const &param{GetParam()};

  auto const storage_sz{param.strip_sz * param.stripes_nr};
  auto const storages{
      ut::make_zeroed_storages<std::byte>(storage_sz, backend_ctxs_.size()),
  };
  auto const storage_spans{ut::make_storage_spans(storages, storage_sz)};

  auto const hs =
      std::views::all(backend_ctxs_) |
      std::views::transform([](auto const &hs_ctx) { return hs_ctx.h; });

  auto const buf_sz{
      param.strips_per_stripe_nr * param.strip_sz * param.stripes_nr,
  };
  auto const buf{ut::make_unique_random_bytes(buf_sz)};
  auto const buf_span{std::as_bytes(std::span{buf.get(), buf_sz})};

  auto const stripe_sz{param.strip_sz * param.strips_per_stripe_nr};

  for (size_t off{0}; off < buf_span.size(); off += stripe_sz) {
    /* clang-format off */
    for (auto const &[h, storage_span] : std::views::zip(std::views::all(hs), storage_spans)) {
      EXPECT_CALL(*h, submit(An<std::shared_ptr<write_query>>()))
          .WillOnce(ut::make_inmem_writer(storage_span));
    }
    /* clang-format on */
    target_->process(write_query::create(
        buf_span.subspan(off, stripe_sz), off,
        [](write_query const &wq) { EXPECT_EQ(wq.err(), 0); }));
  }

  for (size_t off{0}; off < buf_span.size(); off += param.strip_sz) {
    auto const sid{(off / param.strip_sz) % param.strips_per_stripe_nr};
    auto const soff{
        off / (param.strip_sz * param.strips_per_stripe_nr) * param.strip_sz,
    };
    auto const s1{buf_span.subspan(off, param.strip_sz)};
    auto const s2{
        std::as_bytes(storage_spans[sid].subspan(soff, param.strip_sz)),
    };
    EXPECT_THAT(s1, ElementsAreArray(s2));
  }
}

INSTANTIATE_TEST_SUITE_P(RAID0_Operations, RAID0,
                         Values(
                             RAID0Param{
                                 .strip_sz = 512,
                                 .strips_per_stripe_nr = 2,
                                 .stripes_nr = 4,
                             },
                             RAID0Param{
                                 .strip_sz = 4_KiB,
                                 .strips_per_stripe_nr = 1,
                                 .stripes_nr = 10,
                             },
                             RAID0Param{
                                 .strip_sz = 128_KiB,
                                 .strips_per_stripe_nr = 8,
                                 .stripes_nr = 6,
                             }));
