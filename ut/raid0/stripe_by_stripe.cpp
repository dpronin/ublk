#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>

#include <algorithm>
#include <memory>
#include <ranges>
#include <span>

#include "mm/mem.hpp"

#include "utils/size_units.hpp"

#include "read_query.hpp"
#include "write_query.hpp"

#include "base.hpp"

using namespace ublk;
using namespace testing;

namespace {

struct stripe_by_stripe_param {
  ut::raid0::backend_cfg be_cfg;
  size_t stripes_nr;
};

class StripeByStripe : public ut::raid0::Base<stripe_by_stripe_param> {};

} // namespace

TEST_P(StripeByStripe, Read) {
  auto const &param{GetParam()};

  auto const &target_cfg{param.be_cfg};

  auto const hs{
      std::views::all(backend_ctxs_) |
          std::views::transform([](auto const &hs_ctx) { return hs_ctx.h; }),
  };

  auto const stripe_sz{target_cfg.strip_sz * target_cfg.strips_per_stripe_nr};

  for (auto stripe_id{0uz}; stripe_id < param.stripes_nr; ++stripe_id) {
    auto const stripe_buf{mm::make_unique_zeroed_bytes(stripe_sz)};
    auto const stripe_buf_span{
        std::as_writable_bytes(std::span{stripe_buf.get(), stripe_sz}),
    };
    auto const stripe_storage_buf{mm::make_unique_random_bytes(stripe_sz)};
    auto const stripe_storage_buf_span{
        std::as_bytes(std::span{stripe_storage_buf.get(), stripe_sz}),
    };

    ASSERT_THAT(stripe_buf_span,
                Not(ElementsAreArray(stripe_storage_buf_span)));

    for (auto strip_id{0uz}; auto const &h : hs) {
      auto const strip_storage_buf_span{
          stripe_storage_buf_span.subspan((strip_id++) * target_cfg.strip_sz,
                                          target_cfg.strip_sz),
      };
      EXPECT_CALL(*h, submit(An<std::shared_ptr<read_query>>()))
          .WillOnce([=, &target_cfg](std::shared_ptr<read_query> rq) {
            EXPECT_TRUE(rq);
            EXPECT_EQ(rq->offset(), target_cfg.strip_sz * stripe_id);
            EXPECT_EQ(rq->buf().size(), strip_storage_buf_span.size());
            std::ranges::copy(strip_storage_buf_span, rq->buf().begin());
            return 0;
          });
    }

    auto const res{
        be_->process(read_query::create(
            stripe_buf_span, stripe_id * stripe_sz,
            [](read_query const &rq) { EXPECT_EQ(rq.err(), 0); })),
    };

    EXPECT_EQ(res, 0);

    EXPECT_THAT(stripe_buf_span, ElementsAreArray(stripe_storage_buf_span));
  }
}

TEST_P(StripeByStripe, Write) {
  auto const &param{GetParam()};

  auto const &target_cfg{param.be_cfg};

  auto const hs{
      std::views::all(backend_ctxs_) |
          std::views::transform([](auto const &hs_ctx) { return hs_ctx.h; }),
  };

  auto const stripe_sz{target_cfg.strip_sz * target_cfg.strips_per_stripe_nr};

  for (auto stripe_id{0uz}; stripe_id < param.stripes_nr; ++stripe_id) {
    auto const stripe_buf{mm::make_unique_random_bytes(stripe_sz)};
    auto const stripe_buf_span{
        std::as_bytes(std::span{stripe_buf.get(), stripe_sz}),
    };
    auto const stripe_storage_buf{mm::make_unique_zeroed_bytes(stripe_sz)};
    auto const stripe_storage_buf_span{
        std::as_writable_bytes(std::span{stripe_storage_buf.get(), stripe_sz}),
    };

    ASSERT_THAT(stripe_buf_span,
                Not(ElementsAreArray(stripe_storage_buf_span)));

    for (auto strip_id{0uz}; auto const &h : hs) {
      auto const strip_storage_buf_span{
          stripe_storage_buf_span.subspan((strip_id++) * target_cfg.strip_sz,
                                          target_cfg.strip_sz),
      };
      EXPECT_CALL(*h, submit(An<std::shared_ptr<write_query>>()))
          .WillOnce([=, &target_cfg](std::shared_ptr<write_query> wq) {
            EXPECT_TRUE(wq);
            EXPECT_EQ(wq->offset(), target_cfg.strip_sz * stripe_id);
            EXPECT_EQ(wq->buf().size(), strip_storage_buf_span.size());
            std::ranges::copy(wq->buf(), strip_storage_buf_span.begin());
            return 0;
          });
    }

    auto const res{
        be_->process(write_query::create(
            stripe_buf_span, stripe_id * stripe_sz,
            [](write_query const &wq) { EXPECT_EQ(wq.err(), 0); })),
    };

    EXPECT_EQ(res, 0);

    EXPECT_THAT(stripe_buf_span, ElementsAreArray(stripe_storage_buf_span));
  }
}

INSTANTIATE_TEST_SUITE_P(RAID0, StripeByStripe,
                         Values(
                             stripe_by_stripe_param{
                                 .be_cfg =
                                     {
                                         .strip_sz = 512uz,
                                         .strips_per_stripe_nr = 2uz,
                                     },
                                 .stripes_nr = 4uz,
                             },
                             stripe_by_stripe_param{
                                 .be_cfg =
                                     {
                                         .strip_sz = 4_KiB,
                                         .strips_per_stripe_nr = 1uz,
                                     },
                                 .stripes_nr = 10uz,
                             },
                             stripe_by_stripe_param{
                                 .be_cfg =
                                     {
                                         .strip_sz = 128_KiB,
                                         .strips_per_stripe_nr = 8uz,
                                     },
                                 .stripes_nr = 6uz,
                             }));
