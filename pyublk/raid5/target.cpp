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

namespace ublk::raid5 {

Target::Target(uint64_t strip_sz, std::vector<std::shared_ptr<IRWHandler>> hs)
    : strip_sz_(strip_sz), hs_(std::move(hs)) {
  assert(is_power_of_2(strip_sz_));
  assert(is_multiple_of(strip_sz_, kCachedStripeAlignment));
  assert(!(hs_.size() < 2));
  assert(std::ranges::all_of(
      hs_, [](auto const &h) { return static_cast<bool>(h); }));

  stripe_data_sz_ = strip_sz_ * (hs_.size() - 1);

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
    auto const stripe_id = strip_id / (hs_.size() - 1);
    auto const parity_strip_id = stripe_id % hs_.size();

    auto strip_in_stripe_id = (strip_id + stripe_id) % hs_.size();
    if (parity_strip_id <= strip_in_stripe_id) {
      ++strip_in_stripe_id;
      if (parity_strip_id == (hs_.size() - 1))
        ++strip_in_stripe_id;
    }
    strip_in_stripe_id %= hs_.size();

    auto const h_offset = strip_offset + stripe_id * strip_sz_;
    auto const hid = strip_in_stripe_id;
    auto const &h = hs_[hid];

    if (auto const res = h->read(chunk, h_offset); res >= 0) [[likely]] {
      rb += res;
    } else {
      return res;
    }

    strip_offset = 0;
    ++strip_id;
    buf = buf.subspan(chunk.size());
  }

  return rb;
}

void Target::parity_renew(uint64_t parity_strip_id,
                          std::span<std::byte> stripe) noexcept {
  auto const parity_offset = (parity_strip_id % hs_.size()) * strip_sz_;
  auto const data_fp =
      to_span_of<uint64_t const>(stripe.subspan(0, parity_offset));
  auto const parity_dst =
      to_span_of<uint64_t>(stripe.subspan(parity_offset, strip_sz_));
  auto const data_lp = to_span_of<uint64_t const>(
      stripe.subspan(parity_offset + parity_dst.size_bytes()));
  algo::fill(parity_dst, UINT64_C(0));
  for (auto data_src : {data_fp, data_lp}) {
    for (auto in = data_src; !in.empty(); in = in.subspan(parity_dst.size())) {
      math::xor_to(in.subspan(0, parity_dst.size()), parity_dst);
    }
  }
}

void Target::cached_stripe_parity_renew(uint64_t parity_strip_id) noexcept {
  parity_renew(parity_strip_id, cached_stripe_view());
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
  ssize_t rb{0};

  auto stripe_id{offset / stripe_data_sz_};
  auto stripe_offset{offset % stripe_data_sz_};

  while (!buf.empty()) {
    auto const parity_strip_id = stripe_id % hs_.size();
    auto const parity_stripe_offset = parity_strip_id * strip_sz_;

    auto const chunk{
        buf.subspan(0, std::min(stripe_data_sz_ - stripe_offset, buf.size())),
    };

    auto const data_fp = cached_stripe_view().subspan(0, parity_stripe_offset);
    auto const data_lp =
        cached_stripe_view().subspan(parity_stripe_offset + strip_sz_);

    if (chunk.size() < data_fp.size() + data_lp.size()) {
      if (auto const res =
              read_data_skip_parity(stripe_id * (hs_.size() - 1), 0, data_fp);
          res < 0) [[unlikely]] {
        return res;
      }

      if (auto const res = read_data_skip_parity(stripe_id * (hs_.size() - 1) +
                                                     data_fp.size() / strip_sz_,
                                                 0, data_lp);
          res < 0) [[unlikely]] {
        return res;
      }
    }

    /* Modify the part of the stripe with the new data come in */
    auto const data_fp_offset = std::min(stripe_offset, data_fp.size());
    auto const chunk_fp = chunk.subspan(
        0, std::min(chunk.size(), data_fp.size() - data_fp_offset));
    algo::copy(std::as_bytes(chunk_fp),
               data_fp.subspan(data_fp_offset, chunk_fp.size()));
    auto const data_lp_offset =
        std::max(stripe_offset, data_fp.size()) - data_fp.size();
    auto const chunk_lp = chunk.subspan(chunk_fp.size());
    algo::copy(std::as_bytes(chunk_lp),
               data_lp.subspan(data_lp_offset, chunk_lp.size()));

    /* Renew Parity of the stripe */
    cached_stripe_parity_renew(parity_strip_id);

    /* Write Back the whole stripe including the parity part */
    if (auto const res = cached_stripe_write(stripe_id); res < 0) [[unlikely]] {
      return res;
    }

    ++stripe_id;
    stripe_offset = 0;
    buf = buf.subspan(chunk.size());
    rb += chunk.size();
  }

  return rb;
}

} // namespace ublk::raid5
