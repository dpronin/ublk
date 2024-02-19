#include "target.hpp"

#include <cassert>
#include <cstdint>

#include <algorithm>
#include <utility>

#include "utility.hpp"

namespace ublk::raid0 {

Target::Target(uint64_t strip_sz, std::vector<std::shared_ptr<IRWHandler>> hs)
    : strip_sz_(strip_sz), hs_(std::move(hs)) {
  assert(is_power_of_2(strip_sz_));
  assert(!hs_.empty());
  assert(std::ranges::all_of(
      hs_, [](auto const &h) { return static_cast<bool>(h); }));
}

ssize_t Target::read(uint64_t strip_id_from, uint64_t strip_offset_from,
                     std::span<std::byte> buf) noexcept {
  ssize_t rb{0};

  auto strip_id = strip_id_from + strip_offset_from / strip_sz_;
  auto strip_offset = strip_offset_from % strip_sz_;

  while (!buf.empty()) {
    auto const strip_id_in_stripe = strip_id % hs_.size();
    auto const hid = strip_id_in_stripe;
    auto const &h = hs_[hid];
    auto const stripe_id = strip_id / hs_.size();
    auto const h_offset = strip_offset + stripe_id * strip_sz_;
    auto const chunk =
        buf.subspan(0, std::min(strip_sz_ - strip_offset, buf.size()));

    if (auto const res = h->read(chunk, h_offset); res >= 0) [[likely]] {
      rb += res;
      strip_offset = 0;
      ++strip_id;
      buf = buf.subspan(chunk.size());
    } else {
      return res;
    }
  }

  return rb;
}

ssize_t Target::read(std::span<std::byte> buf, __off64_t offset) noexcept {
  return read(offset / strip_sz_, offset % strip_sz_, buf);
}

ssize_t Target::write(uint64_t strip_id_from, uint64_t strip_offset_from,
                      std::span<std::byte const> buf) noexcept {
  ssize_t rb{0};

  auto strip_id = strip_id_from + strip_offset_from / strip_sz_;
  auto strip_offset = strip_offset_from % strip_sz_;

  while (!buf.empty()) {
    auto const strip_id_in_stripe = strip_id % hs_.size();
    auto const hid = strip_id_in_stripe;
    auto const &h = hs_[hid];
    auto const stripe_id = strip_id / hs_.size();
    auto const h_offset = strip_offset + stripe_id * strip_sz_;
    auto const chunk =
        buf.subspan(0, std::min(strip_sz_ - strip_offset, buf.size()));

    if (auto const res = h->write(chunk, h_offset); res >= 0) [[likely]] {
      rb += res;
      strip_offset = 0;
      ++strip_id;
      buf = buf.subspan(chunk.size());
    } else {
      return res;
    }
  }

  return rb;
}

ssize_t Target::write(std::span<std::byte const> buf,
                      __off64_t offset) noexcept {
  return write(offset / strip_sz_, offset % strip_sz_, buf);
}

} // namespace ublk::raid0
