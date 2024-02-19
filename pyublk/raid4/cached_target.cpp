#include "cached_target.hpp"

#include <cassert>
#include <cstdint>

#include <algorithm>
#include <memory>
#include <span>
#include <utility>

#include "algo.hpp"
#include "span.hpp"

namespace ublk::raid4 {

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
    auto const [cached_stripe, valid] =
        cache_->find_allocate_mutable(stripe_id);
    assert(!cached_stripe.empty());
    auto const cached_data_stripe = cached_stripe.subspan(0, stripe_data_sz_);
    if (!valid) {
      /*
       * Read the whole stripe from the backend excluding parity if cache miss
       * took place
       */
      if (auto const res = read_data_skip_parity(stripe_id * (hs_.size() - 1),
                                                 0, cached_data_stripe);
          res < 0) [[unlikely]] {
        cache_->invalidate(stripe_id);
        return res;
      }
    }

    auto const chunk{
        buf.subspan(0, std::min(cached_data_stripe.size_bytes() - stripe_offset,
                                buf.size())),
    };

    auto const from = chunk;
    auto const to = cached_data_stripe.subspan(stripe_offset, chunk.size());

    /* Modify the part of the stripe with the new data come in */
    algo::copy(from, to);

    /* Renew Parity of the stripe */
    parity_renew(cached_stripe);

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
    if (auto const cached_stripe_data =
            cached_stripe.subspan(0, stripe_data_sz_);
        valid) {
      auto const chunk{
          buf.subspan(0,
                      std::min(cached_stripe_data.size_bytes() - stripe_offset,
                               buf.size())),
      };

      auto const from = cached_stripe_data.subspan(stripe_offset, chunk.size());
      auto const to = chunk;

      algo::copy(to_span_of<std::byte const>(from), to);

      ++stripe_id;
      stripe_offset = 0;
      buf = buf.subspan(chunk.size());
      rb += chunk.size();
    } else if (auto const res = Target::read_data_skip_parity(
                   stripe_id * (hs_.size() - 1), 0, cached_stripe_data);
               res < 0) [[unlikely]] {
      cache_->invalidate(stripe_id);
      return res;
    } else {
      /* Renew Parity of the stripe */
      parity_renew(cached_stripe);
    }
  }

  return rb;
}

} // namespace ublk::raid4
