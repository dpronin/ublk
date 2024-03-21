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

struct ChunkByChunkParam {
  ut::raid0::target_cfg target_cfg;
  size_t start_off;
  size_t chunk_sz;
};

class ChunkByChunk : public ut::raid0::Base<ChunkByChunkParam> {};

} // namespace

TEST_P(ChunkByChunk, Read) {
  auto const &param{GetParam()};

  auto const &target_cfg{param.target_cfg};

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

    for (size_t chunk_off{0}; chunk_off < chunk_buf_span.size();) {
      auto const req_off{off + chunk_off};
      auto const strip_off{req_off % target_cfg.strip_sz};
      auto const req_sz{
          std::min(chunk_buf_span.size() - chunk_off,
                   target_cfg.strip_sz - strip_off),
      };
      auto const req_storage_span{
          raid_storage_buf_span.subspan(req_off, req_sz),
      };
      auto const strip_id{req_off / target_cfg.strip_sz};
      auto const stripe_id{strip_id / target_cfg.strips_per_stripe_nr};
      auto const hid{strip_id % target_cfg.strips_per_stripe_nr};
      EXPECT_CALL(*hs[hid], submit(An<std::shared_ptr<read_query>>()))
          .WillOnce([=, &target_cfg](std::shared_ptr<read_query> rq) {
            EXPECT_TRUE(rq);
            EXPECT_EQ(rq->offset(),
                      stripe_id * target_cfg.strip_sz + strip_off);
            EXPECT_EQ(rq->buf().size(), req_storage_span.size());
            std::ranges::copy(req_storage_span, rq->buf().begin());
            return 0;
          });
      chunk_off += req_storage_span.size();
    }

    target_->process(
        read_query::create(chunk_buf_span, off, [](read_query const &rq) {
          EXPECT_EQ(rq.err(), 0);
        }));

    EXPECT_THAT(chunk_buf_span, ElementsAreArray(chunk_raid_storage_buf_span));
  }
}

TEST_P(ChunkByChunk, Write) {
  auto const &param{GetParam()};

  auto const &target_cfg{param.target_cfg};

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

    for (size_t chunk_off{0}; chunk_off < chunk_buf_span.size();) {
      auto const req_off{off + chunk_off};
      auto const strip_off{req_off % target_cfg.strip_sz};
      auto const req_sz{
          std::min(chunk_buf_span.size() - chunk_off,
                   target_cfg.strip_sz - strip_off),
      };
      auto const req_storage_span{
          raid_storage_buf_span.subspan(req_off, req_sz),
      };
      auto const strip_id{req_off / target_cfg.strip_sz};
      auto const stripe_id{strip_id / target_cfg.strips_per_stripe_nr};
      auto const hid{strip_id % target_cfg.strips_per_stripe_nr};
      EXPECT_CALL(*hs[hid], submit(An<std::shared_ptr<write_query>>()))
          .WillOnce([=, &target_cfg](std::shared_ptr<write_query> wq) {
            EXPECT_TRUE(wq);
            EXPECT_EQ(wq->offset(),
                      stripe_id * target_cfg.strip_sz + strip_off);
            EXPECT_EQ(wq->buf().size(), req_storage_span.size());
            std::ranges::copy(wq->buf(), req_storage_span.begin());
            return 0;
          });
      chunk_off += req_storage_span.size();
    }

    target_->process(
        write_query::create(chunk_buf_span, off, [](write_query const &wq) {
          EXPECT_EQ(wq.err(), 0);
        }));

    EXPECT_THAT(chunk_buf_span, ElementsAreArray(chunk_raid_storage_buf_span));
  }
}

INSTANTIATE_TEST_SUITE_P(RAID0, ChunkByChunk,
                         Values(
                             ChunkByChunkParam{
                                 .target_cfg =
                                     {
                                         .strip_sz = 512,
                                         .strips_per_stripe_nr = 2,
                                         .stripes_nr = 4,
                                     },
                                 .start_off = 0,
                                 .chunk_sz = 512,
                             },
                             ChunkByChunkParam{
                                 .target_cfg =
                                     {
                                         .strip_sz = 4_KiB,
                                         .strips_per_stripe_nr = 3,
                                         .stripes_nr = 10,
                                     },
                                 .start_off = 512,
                                 .chunk_sz = 1_KiB,
                             },
                             ChunkByChunkParam{
                                 .target_cfg =
                                     {
                                         .strip_sz = 4_KiB,
                                         .strips_per_stripe_nr = 3,
                                         .stripes_nr = 10,
                                     },
                                 .start_off = 3_KiB,
                                 .chunk_sz = 6_KiB,
                             }));
