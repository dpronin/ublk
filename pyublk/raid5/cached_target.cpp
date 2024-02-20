#include "cached_target.hpp"

#include <cassert>
#include <cstdint>

#include <algorithm>
#include <memory>
#include <span>
#include <utility>

#include "algo.hpp"

namespace ublk::raid5 {

CachedTarget::CachedTarget(
    uint64_t strip_sz,
    std::unique_ptr<flat_lru_cache<uint64_t, std::byte>> cache,
    std::vector<std::shared_ptr<IRWHandler>> hs)
    : Target(strip_sz, std::move(hs)), cache_(std::move(cache)) {
  assert(cache_);
  assert(cache_->item_sz() == (stripe_data_sz_ + strip_sz_));
}

ssize_t CachedTarget::write(std::span<std::byte const> buf,
                            __off64_t offset) noexcept {
  ssize_t rb{0};

  auto stripe_id{offset / stripe_data_sz_};
  auto stripe_offset{offset % stripe_data_sz_};

  while (!buf.empty()) {
    auto const chunk{
        buf.subspan(0, std::min(stripe_data_sz_ - stripe_offset, buf.size())),
    };

    auto [cached_stripe, valid] = cache_->find_allocate_mutable(stripe_id);
    assert(!cached_stripe.empty());

    auto const parity_strip_id = stripe_id % hs_.size();
    auto const parity_stripe_offset = parity_strip_id * strip_sz_;

    /* Split data and parity strips */
    auto const data_fp = cached_stripe.subspan(0, parity_stripe_offset);
    auto const data_lp =
        cached_stripe.subspan(parity_stripe_offset + strip_sz_);

    if (!valid && chunk.size() < (data_fp.size() + data_lp.size())) {
      /*
       * Read the whole stripe from backend if not found as a valid stripe line
       * in the cache
       */

      /* Read the first part of the stripe before the parity strip */
      if (auto const res =
              read_data_skip_parity(stripe_id * (hs_.size() - 1), 0, data_fp);
          res < 0) [[unlikely]] {
        cache_->invalidate(stripe_id);
        return res;
      }

      /* Read the last part of the stripe after the parity strip */
      if (auto const res = read_data_skip_parity(stripe_id * (hs_.size() - 1) +
                                                     data_fp.size() / strip_sz_,
                                                 0, data_lp);
          res < 0) [[unlikely]] {
        cache_->invalidate(stripe_id);
        return res;
      }
    }

    /* Modify the part of the stripe with the new data come in */
    auto const data_fp_offset = std::min(stripe_offset, data_fp.size());
    auto const chunk_fp = chunk.subspan(
        0, std::min(chunk.size(), data_fp.size() - data_fp_offset));
    algo::copy(chunk_fp, data_fp.subspan(data_fp_offset, chunk_fp.size()));
    auto const data_lp_offset =
        std::max(stripe_offset, data_fp.size()) - data_fp.size();
    auto const chunk_lp = chunk.subspan(chunk_fp.size());
    algo::copy(chunk_lp, data_lp.subspan(data_lp_offset, chunk_lp.size()));

    /* Renew Parity of the stripe */
    parity_renew(parity_strip_id, cached_stripe);

    /* Write Back the whole stripe including the parity part */
    if (auto const res = stripe_write(stripe_id, cached_stripe); res < 0)
        [[unlikely]] {
      cache_->invalidate(stripe_id);
      return res;
    }

    ++stripe_id;
    stripe_offset = 0;
    buf = buf.subspan(chunk.size());
    rb += chunk.size();
  }

  return rb;
}

ssize_t CachedTarget::read(std::span<std::byte> buf,
                           __off64_t offset) noexcept {
  ssize_t rb{0};

  auto stripe_id{offset / stripe_data_sz_};
  auto stripe_offset{offset % stripe_data_sz_};

  while (!buf.empty()) {
    auto [cached_stripe, valid] = cache_->find_allocate_mutable(stripe_id);
    assert(!cached_stripe.empty());

    auto const parity_strip_id = stripe_id % hs_.size();
    auto const parity_stripe_offset = parity_strip_id * strip_sz_;

    /* Split stripe in data chunks of strips leaving out parity */
    auto const data_fp = cached_stripe.subspan(0, parity_stripe_offset);
    auto const data_lp =
        cached_stripe.subspan(parity_stripe_offset + strip_sz_);

    if (valid) {
      auto const chunk{
          buf.subspan(0, std::min(stripe_data_sz_ - stripe_offset, buf.size())),
      };

      /* Read the part of the stripe to the chunk */
      auto const data_fp_offset = std::min(stripe_offset, data_fp.size());
      auto const chunk_fp = chunk.subspan(
          0, std::min(chunk.size(), data_fp.size() - data_fp_offset));
      algo::copy(
          std::as_bytes(data_fp.subspan(data_fp_offset, chunk_fp.size())),
          chunk_fp);
      auto const data_lp_offset =
          std::max(stripe_offset, data_fp.size()) - data_fp.size();
      auto const chunk_lp = chunk.subspan(chunk_fp.size());
      algo::copy(
          std::as_bytes(data_lp.subspan(data_lp_offset, chunk_lp.size())),
          chunk_lp);

      ++stripe_id;
      stripe_offset = 0;
      buf = buf.subspan(chunk.size());
      rb += chunk.size();
    } else {
      /* Read the first part of the stripe before the parity strip */
      if (auto const res =
              read_data_skip_parity(stripe_id * (hs_.size() - 1), 0, data_fp);
          res < 0) [[unlikely]] {
        cache_->invalidate(stripe_id);
        return res;
      }

      /* Read the last part of the stripe after the parity strip */
      if (auto const res = read_data_skip_parity(stripe_id * (hs_.size() - 1) +
                                                     data_fp.size() / strip_sz_,
                                                 0, data_lp);
          res < 0) [[unlikely]] {
        cache_->invalidate(stripe_id);
        return res;
      }

      /* Renew Parity of the stripe */
      parity_renew(parity_strip_id, cached_stripe);
    }
  }

  return rb;
}

} // namespace ublk::raid5
