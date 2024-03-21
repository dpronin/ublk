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

    auto hs{
        std::views::all(backend_ctxs_) |
            std::views::transform([](auto const &hs_ctx) { return hs_ctx.h; }),
    };

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

  auto const hs{
      std::views::all(backend_ctxs_) |
          std::views::transform([](auto const &hs_ctx) { return hs_ctx.h; }),
  };

  auto const stripe_sz{param.strip_sz * param.strips_per_stripe_nr};

  for (size_t stripe_id{0}; stripe_id < param.stripes_nr; ++stripe_id) {
    auto const stripe_buf{mm::make_unique_zeroed_bytes(stripe_sz)};
    auto const stripe_buf_span{
        std::as_writable_bytes(std::span{stripe_buf.get(), stripe_sz}),
    };
    auto const stripe_storage_buf{mm::make_unique_random_bytes(stripe_sz)};
    auto const stripe_storage_buf_span{
        std::as_bytes(std::span{stripe_storage_buf.get(), stripe_sz}),
    };
    /* clang-format off */
    for (size_t strip_id{0}; auto const &h : hs) {
      auto const strip_storage_buf_span{stripe_storage_buf_span.subspan((strip_id++) * param.strip_sz, param.strip_sz)};
      EXPECT_CALL(*h, submit(An<std::shared_ptr<read_query>>()))
      .WillOnce([=, &param](std::shared_ptr<read_query> rq) {
        EXPECT_TRUE(rq);
        EXPECT_EQ(rq->offset(), param.strip_sz * stripe_id);
        EXPECT_EQ(rq->buf().size(), strip_storage_buf_span.size());
        std::ranges::copy(strip_storage_buf_span, rq->buf().begin());
        return 0;
      });
    }
    /* clang-format on */
    target_->process(read_query::create(
        stripe_buf_span, stripe_id * stripe_sz,
        [](read_query const &rq) { EXPECT_EQ(rq.err(), 0); }));
    EXPECT_THAT(stripe_buf_span, ElementsAreArray(stripe_storage_buf_span));
  }
}

TEST_P(RAID0, FullStripeWriting) {
  auto const &param{GetParam()};

  auto const hs{
      std::views::all(backend_ctxs_) |
          std::views::transform([](auto const &hs_ctx) { return hs_ctx.h; }),
  };

  auto const stripe_sz{param.strip_sz * param.strips_per_stripe_nr};

  for (size_t stripe_id{0}; stripe_id < param.stripes_nr; ++stripe_id) {
    auto const stripe_buf{mm::make_unique_random_bytes(stripe_sz)};
    auto const stripe_buf_span{
        std::as_bytes(std::span{stripe_buf.get(), stripe_sz}),
    };
    auto const stripe_storage_buf{mm::make_unique_zeroed_bytes(stripe_sz)};
    auto const stripe_storage_buf_span{
        std::as_writable_bytes(std::span{stripe_storage_buf.get(), stripe_sz}),
    };
    /* clang-format off */
    for (size_t strip_id{0}; auto const &h : hs) {
      auto const strip_storage_buf_span{stripe_storage_buf_span.subspan((strip_id++) * param.strip_sz, param.strip_sz)};
      EXPECT_CALL(*h, submit(An<std::shared_ptr<write_query>>()))
      .WillOnce([=, &param](std::shared_ptr<write_query> wq) {
        EXPECT_TRUE(wq);
        EXPECT_EQ(wq->offset(), param.strip_sz * stripe_id);
        EXPECT_EQ(wq->buf().size(), strip_storage_buf_span.size());
        std::ranges::copy(wq->buf(), strip_storage_buf_span.begin());
        return 0;
      });
    }
    /* clang-format on */
    target_->process(write_query::create(
        stripe_buf_span, stripe_id * stripe_sz,
        [](write_query const &wq) { EXPECT_EQ(wq.err(), 0); }));
    EXPECT_THAT(stripe_buf_span, ElementsAreArray(stripe_storage_buf_span));
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
