#include "target.hpp"

#include <cassert>
#include <cstdint>

#include <algorithm>
#include <span>
#include <utility>

#include "algo.hpp"
#include "math.hpp"
#include "mem.hpp"
#include "utility.hpp"

namespace ublk::raid4 {

Target::Target(uint64_t strip_sz, std::vector<std::shared_ptr<IRWHandler>> hs)
    : strip_sz_(strip_sz), stripe_data_sz_(strip_sz_ * (hs.size() - 1)),
      hs_(std::move(hs)) {
  assert(is_power_of_2(strip_sz_));
  assert(is_multiple_of(strip_sz_, kCachedStripeAlignment));
  assert(!(hs_.size() < 2));
  assert(std::ranges::all_of(
      hs_, [](auto const &h) { return static_cast<bool>(h); }));

  cached_stripe_ = get_unique_bytes_generator(stripe_data_sz_ + strip_sz_)();
}

ssize_t Target::read_data_skip_parity(uint64_t strip_id_from,
                                      uint64_t strip_offset_from,
                                      std::span<std::byte> buf) noexcept {
  ssize_t rb{0};

  auto strip_id{strip_id_from + strip_offset_from / strip_sz_};
  auto strip_offset{strip_offset_from % strip_sz_};

  while (!buf.empty()) {
    auto const chunk =
        buf.subspan(0, std::min(strip_sz_ - strip_offset, buf.size()));

    auto const strip_in_stripe_id = strip_id % (hs_.size() - 1);
    auto const stripe_id = strip_id / (hs_.size() - 1);
    auto const h_offset = strip_offset + stripe_id * strip_sz_;
    auto const hid = strip_in_stripe_id;
    auto const &h = hs_[hid];

    if (auto const res = h->read(chunk, h_offset); res >= 0) [[likely]] {
      rb += res;
      strip_offset = 0;
      ++strip_id;
      buf = buf.subspan(res);
    } else {
      return res;
    }
  }

  return rb;
}

ssize_t Target::read_stripe_data(uint64_t stripe_id, uint64_t stripe_offset,
                                 std::span<std::byte> buf) noexcept {
  return read_data_skip_parity(
      stripe_id * (hs_.size() - 1) +
          (stripe_offset % stripe_data_sz_) / strip_sz_,
      stripe_offset % strip_sz_,
      buf.subspan(0, std::min(stripe_data_sz_, buf.size())));
}

ssize_t Target::read_stripe_data(uint64_t stripe_id,
                                 std::span<std::byte> buf) noexcept {
  return read_stripe_data(stripe_id, 0, buf);
}

ssize_t Target::read_stripe_parity(uint64_t stripe_id,
                                   std::span<std::byte> buf) noexcept {
  return hs_.back()->read(buf.subspan(0, std::min(strip_sz_, buf.size())),
                          stripe_id * strip_sz_);
}

void Target::parity_renew(std::span<std::byte const> stripe_data,
                          std::span<std::byte> parity) noexcept {
  assert(!(stripe_data.size() < stripe_data_sz_));
  assert(!(parity.size() < strip_sz_));

  stripe_data = stripe_data.subspan(0, stripe_data_sz_);
  parity = parity.subspan(0, strip_sz_);

  auto const parity_u64 = to_span_of<uint64_t>(parity);
  algo::fill(parity_u64, UINT64_C(0));
  for (auto stripe_data_u64 = to_span_of<uint64_t const>(stripe_data);
       !stripe_data_u64.empty();
       stripe_data_u64 = stripe_data_u64.subspan(parity_u64.size())) {
    math::xor_to(stripe_data_u64.subspan(0, parity_u64.size()), parity_u64);
  }
}

void Target::cached_stripe_parity_renew() noexcept {
  parity_renew(cached_stripe_data_view(), cached_stripe_parity_view());
}

ssize_t Target::stripe_write(uint64_t stripe_id_at,
                             std::span<std::byte const> stripe) noexcept {
  ssize_t wb{0};

  for (size_t hid = 0; hid < hs_.size(); ++hid) {
    if (auto const res =
            hs_[hid]->write(stripe.subspan(hid * strip_sz_, strip_sz_),
                            stripe_id_at * strip_sz_);
        res >= 0) [[likely]] {
      wb += res;
    } else {
      return res;
    }
  }

  return wb;
}

ssize_t Target::cached_stripe_write(uint64_t stripe_id_at) noexcept {
  return stripe_write(stripe_id_at, cached_stripe_view());
}

ssize_t Target::read(std::span<std::byte> buf, __off64_t offset) noexcept {
  return read_data_skip_parity(offset / strip_sz_, offset % strip_sz_, buf);
}

ssize_t Target::write(std::span<std::byte const> buf,
                      __off64_t offset) noexcept {
  ssize_t wb{0};

  auto stripe_id{offset / stripe_data_sz_};
  auto stripe_offset{offset % stripe_data_sz_};

  while (!buf.empty()) {
    auto const chunk{
        buf.subspan(0, std::min(stripe_data_sz_ - stripe_offset, buf.size())),
    };

    auto const copy_from = chunk;

    /*
     * Read the whole stripe from the backend excluding parity in case we intend
     * to partially modify the stripe
     */
    if (copy_from.size() < cached_stripe_data_view().size()) {
      if (auto const res =
              read_stripe_data(stripe_id, cached_stripe_data_view());
          res < 0) [[unlikely]] {
        return res;
      }
    }

    auto const copy_to =
        cached_stripe_data_view().subspan(stripe_offset, chunk.size());

    /* Modify the part of the stripe with the new data come in */
    algo::copy(copy_from, copy_to);

    /* Renew Parity of the stripe */
    cached_stripe_parity_renew();

    /* Write Back the whole stripe including the parity part */
    if (auto const res = cached_stripe_write(stripe_id); res < 0) [[unlikely]] {
      return res;
    }

    ++stripe_id;
    stripe_offset = 0;
    buf = buf.subspan(chunk.size());
    wb += chunk.size();
  }

  return wb;
}

} // namespace ublk::raid4
