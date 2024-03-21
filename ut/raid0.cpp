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

struct target_cfg {
  size_t strip_sz;
  size_t strips_per_stripe_nr;
  size_t stripes_nr;
};

template <typename ParamType>
class RAID0BaseTest : public TestWithParam<ParamType> {
protected:
  void SetUp() override {
    auto const &param{this->GetParam()};

    auto const &target_cfg{param.target_cfg};

    backend_ctxs_.resize(target_cfg.strips_per_stripe_nr);
    std::ranges::generate(backend_ctxs_, [] -> backend_ctx {
      return {
          .h = std::make_shared<ut::MockRWHandler>(),
      };
    });

    auto hs{
        std::views::all(backend_ctxs_) |
            std::views::transform([](auto const &hs_ctx) { return hs_ctx.h; }),
    };

    target_ = std::make_unique<raid0::Target>(target_cfg.strip_sz, hs);
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

struct RAID0StripeByStripeParam {
  target_cfg target_cfg;
};

class RAID0StripeByStripe : public RAID0BaseTest<RAID0StripeByStripeParam> {};

TEST_P(RAID0StripeByStripe, Read) {
  auto const &param{GetParam()};

  auto const &target_cfg{param.target_cfg};

  auto const hs{
      std::views::all(backend_ctxs_) |
          std::views::transform([](auto const &hs_ctx) { return hs_ctx.h; }),
  };

  auto const stripe_sz{target_cfg.strip_sz * target_cfg.strips_per_stripe_nr};

  for (size_t stripe_id{0}; stripe_id < target_cfg.stripes_nr; ++stripe_id) {
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
    /* clang-format off */
    for (size_t strip_id{0}; auto const &h : hs) {
      auto const strip_storage_buf_span{stripe_storage_buf_span.subspan((strip_id++) * target_cfg.strip_sz, target_cfg.strip_sz)};
      EXPECT_CALL(*h, submit(An<std::shared_ptr<read_query>>()))
      .WillOnce([=, &target_cfg](std::shared_ptr<read_query> rq) {
        EXPECT_TRUE(rq);
        EXPECT_EQ(rq->offset(), target_cfg.strip_sz * stripe_id);
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

TEST_P(RAID0StripeByStripe, Write) {
  auto const &param{GetParam()};

  auto const &target_cfg{param.target_cfg};

  auto const hs{
      std::views::all(backend_ctxs_) |
          std::views::transform([](auto const &hs_ctx) { return hs_ctx.h; }),
  };

  auto const stripe_sz{target_cfg.strip_sz * target_cfg.strips_per_stripe_nr};

  for (size_t stripe_id{0}; stripe_id < target_cfg.stripes_nr; ++stripe_id) {
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
    /* clang-format off */
    for (size_t strip_id{0}; auto const &h : hs) {
      auto const strip_storage_buf_span{stripe_storage_buf_span.subspan((strip_id++) * target_cfg.strip_sz, target_cfg.strip_sz)};
      EXPECT_CALL(*h, submit(An<std::shared_ptr<write_query>>()))
      .WillOnce([=, &target_cfg](std::shared_ptr<write_query> wq) {
        EXPECT_TRUE(wq);
        EXPECT_EQ(wq->offset(), target_cfg.strip_sz * stripe_id);
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

INSTANTIATE_TEST_SUITE_P(RAID0, RAID0StripeByStripe,
                         Values(
                             RAID0StripeByStripeParam{
                                 .target_cfg =
                                     {
                                         .strip_sz = 512,
                                         .strips_per_stripe_nr = 2,
                                         .stripes_nr = 4,
                                     },
                             },
                             RAID0StripeByStripeParam{
                                 .target_cfg =
                                     {
                                         .strip_sz = 4_KiB,
                                         .strips_per_stripe_nr = 1,
                                         .stripes_nr = 10,
                                     },
                             },
                             RAID0StripeByStripeParam{
                                 .target_cfg =
                                     {
                                         .strip_sz = 128_KiB,
                                         .strips_per_stripe_nr = 8,
                                         .stripes_nr = 6,
                                     },
                             }));

struct RAID0ChunkByChunkParam {
  target_cfg target_cfg;
  size_t start_off;
  size_t chunk_sz;
};

class RAID0ChunkByChunkTest : public RAID0BaseTest<RAID0ChunkByChunkParam> {};

TEST_P(RAID0ChunkByChunkTest, Read) {
  auto const &param{GetParam()};

  auto const &target_cfg = param.target_cfg;

  auto const hs{
      std::views::all(backend_ctxs_) |
          std::views::transform([](auto const &hs_ctx) { return hs_ctx.h; }),
  };

  auto const raid_storage_sz{
      target_cfg.strip_sz * target_cfg.strips_per_stripe_nr *
          target_cfg.stripes_nr,
  };
  auto const raid_storage_buf{mm::make_unique_random_bytes(raid_storage_sz)};
  auto const raid_storage_buf_span{
      std::as_bytes(std::span{raid_storage_buf.get(), raid_storage_sz}),
  };

  auto const stripe_sz{target_cfg.strip_sz * target_cfg.strips_per_stripe_nr};

  for (size_t off{param.start_off}; off < raid_storage_sz;
       off += param.chunk_sz) {
    auto const chunk_sz{
        std::min({param.chunk_sz, stripe_sz, raid_storage_sz - off}),
    };
    auto const chunk_buf{mm::make_unique_zeroed_bytes(chunk_sz)};
    auto const chunk_buf_span{
        std::as_writable_bytes(std::span{chunk_buf.get(), chunk_sz}),
    };

    auto const chunk_raid_storage_buf_span{
        raid_storage_buf_span.subspan(off, chunk_buf_span.size()),
    };

    ASSERT_THAT(chunk_buf_span,
                Not(ElementsAreArray(chunk_raid_storage_buf_span)));

    /* clang-format off */
    for (size_t chunk_off{0}; chunk_off < chunk_buf_span.size();) {
      auto const req_off{off + chunk_off};
      auto const strip_off{req_off % target_cfg.strip_sz};
      auto const req_sz{std::min(chunk_buf_span.size() - chunk_off, target_cfg.strip_sz - strip_off)};
      auto const req_storage_span{raid_storage_buf_span.subspan(req_off, req_sz)};
      auto const strip_id{req_off / target_cfg.strip_sz};
      auto const stripe_id{strip_id / target_cfg.strips_per_stripe_nr};
      auto const hid{strip_id % target_cfg.strips_per_stripe_nr};
      EXPECT_CALL(*hs[hid], submit(An<std::shared_ptr<read_query>>()))
      .WillOnce([=, &target_cfg](std::shared_ptr<read_query> rq) {
        EXPECT_TRUE(rq);
        EXPECT_EQ(rq->offset(), stripe_id * target_cfg.strip_sz + strip_off);
        EXPECT_EQ(rq->buf().size(), req_storage_span.size());
        std::ranges::copy(req_storage_span, rq->buf().begin());
        return 0;
      });
      chunk_off += req_storage_span.size();
    }
    /* clang-format on */

    target_->process(
        read_query::create(chunk_buf_span, off, [](read_query const &rq) {
          EXPECT_EQ(rq.err(), 0);
        }));

    EXPECT_THAT(chunk_buf_span, ElementsAreArray(chunk_raid_storage_buf_span));
  }
}

TEST_P(RAID0ChunkByChunkTest, Write) {
  auto const &param{GetParam()};

  auto const &target_cfg = param.target_cfg;

  auto const hs{
      std::views::all(backend_ctxs_) |
          std::views::transform([](auto const &hs_ctx) { return hs_ctx.h; }),
  };

  auto const raid_storage_sz{
      target_cfg.strip_sz * target_cfg.strips_per_stripe_nr *
          target_cfg.stripes_nr,
  };
  auto const raid_storage_buf{mm::make_unique_zeroed_bytes(raid_storage_sz)};
  auto const raid_storage_buf_span{
      std::as_writable_bytes(
          std::span{raid_storage_buf.get(), raid_storage_sz}),
  };

  auto const stripe_sz{target_cfg.strip_sz * target_cfg.strips_per_stripe_nr};

  for (size_t off{param.start_off}; off < raid_storage_sz;
       off += param.chunk_sz) {
    auto const chunk_sz{
        std::min({param.chunk_sz, stripe_sz, raid_storage_sz - off}),
    };
    auto const chunk_buf{mm::make_unique_random_bytes(chunk_sz)};
    auto const chunk_buf_span{
        std::as_bytes(std::span{chunk_buf.get(), chunk_sz}),
    };

    auto const chunk_raid_storage_buf_span{
        raid_storage_buf_span.subspan(off, chunk_buf_span.size()),
    };

    ASSERT_THAT(chunk_buf_span,
                Not(ElementsAreArray(chunk_raid_storage_buf_span)));

    /* clang-format off */
    for (size_t chunk_off{0}; chunk_off < chunk_buf_span.size();) {
      auto const req_off{off + chunk_off};
      auto const strip_off{req_off % target_cfg.strip_sz};
      auto const req_sz{std::min(chunk_buf_span.size() - chunk_off, target_cfg.strip_sz - strip_off)};
      auto const req_storage_span{raid_storage_buf_span.subspan(req_off, req_sz)};
      auto const strip_id{req_off / target_cfg.strip_sz};
      auto const stripe_id{strip_id / target_cfg.strips_per_stripe_nr};
      auto const hid{strip_id % target_cfg.strips_per_stripe_nr};
      EXPECT_CALL(*hs[hid], submit(An<std::shared_ptr<write_query>>()))
      .WillOnce([=, &target_cfg](std::shared_ptr<write_query> wq) {
        EXPECT_TRUE(wq);
        EXPECT_EQ(wq->offset(), stripe_id * target_cfg.strip_sz + strip_off);
        EXPECT_EQ(wq->buf().size(), req_storage_span.size());
        std::ranges::copy(wq->buf(), req_storage_span.begin());
        return 0;
      });
      chunk_off += req_storage_span.size();
    }
    /* clang-format on */

    target_->process(
        write_query::create(chunk_buf_span, off, [](write_query const &wq) {
          EXPECT_EQ(wq.err(), 0);
        }));

    EXPECT_THAT(chunk_buf_span, ElementsAreArray(chunk_raid_storage_buf_span));
  }
}

INSTANTIATE_TEST_SUITE_P(RAID0, RAID0ChunkByChunkTest,
                         Values(
                             RAID0ChunkByChunkParam{
                                 .target_cfg =
                                     {
                                         .strip_sz = 512,
                                         .strips_per_stripe_nr = 2,
                                         .stripes_nr = 4,
                                     },
                                 .start_off = 0,
                                 .chunk_sz = 512,
                             },
                             RAID0ChunkByChunkParam{
                                 .target_cfg =
                                     {
                                         .strip_sz = 4_KiB,
                                         .strips_per_stripe_nr = 3,
                                         .stripes_nr = 10,
                                     },
                                 .start_off = 512,
                                 .chunk_sz = 1_KiB,
                             },
                             RAID0ChunkByChunkParam{
                                 .target_cfg =
                                     {
                                         .strip_sz = 4_KiB,
                                         .strips_per_stripe_nr = 3,
                                         .stripes_nr = 10,
                                     },
                                 .start_off = 3_KiB,
                                 .chunk_sz = 6_KiB,
                             }));
