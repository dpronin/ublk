#include "target.hpp"

#include <cassert>
#include <cstdint>

#include <algorithm>
#include <iterator>
#include <span>
#include <utility>

#include "algo.hpp"
#include "math.hpp"
#include "mem.hpp"
#include "utility.hpp"

namespace ublk::raidsp {

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

ssize_t Target::read_data_skip_parity(uint64_t stripe_id_from,
                                      uint64_t stripe_offset_from,
                                      std::span<std::byte> buf) noexcept {
  ssize_t rb{0};

  auto stripe_id{stripe_id_from + stripe_offset_from / stripe_data_sz_};
  auto stripe_offset{stripe_offset_from % stripe_data_sz_};

  while (!buf.empty()) {
    auto chunk{
        buf.subspan(0, std::min(stripe_data_sz_ - stripe_offset, buf.size())),
    };
    auto const chunk_sz = chunk.size();

    auto hs = stripe_id_to_handlers(stripe_id);

    for (size_t hid = stripe_offset / strip_sz_,
                strip_offset = stripe_offset % strip_sz_;
         !chunk.empty() && hid < (hs.size() - 1); ++hid, strip_offset = 0) {
      auto const h_offset = stripe_id * strip_sz_ + strip_offset;
      auto const h_chunk{
          chunk.subspan(0, std::min(strip_sz_ - strip_offset, chunk.size()))};
      if (auto const res = hs[hid]->read(h_chunk, h_offset); res >= 0)
          [[likely]] {
        assert(static_cast<size_t>(res) == h_chunk.size());
        rb += res;
      } else {
        return res;
      }
      chunk = chunk.subspan(h_chunk.size());
    }

    assert(chunk.empty());

    stripe_offset = 0;
    ++stripe_id;
    buf = buf.subspan(chunk_sz);
  }

  return rb;
}

ssize_t Target::read_stripe_data(uint64_t stripe_id, uint64_t stripe_offset,
                                 std::span<std::byte> buf) noexcept {
  return read_data_skip_parity(
      stripe_id, stripe_offset,
      buf.subspan(0, std::min(stripe_data_sz_, buf.size())));
}

ssize_t Target::read_stripe_data(uint64_t stripe_id,
                                 std::span<std::byte> buf) noexcept {
  return read_stripe_data(stripe_id, 0, buf);
}

ssize_t Target::read_stripe_parity(uint64_t stripe_id,
                                   std::span<std::byte> buf) noexcept {
  buf = buf.subspan(0, std::min(strip_sz_, buf.size()));
  assert(!(buf.size() < strip_sz_));
  auto const parity_id = stripe_id_to_parity_id(stripe_id);
  assert(parity_id < hs_.size());
  return hs_[parity_id]->read(buf, stripe_id * strip_sz_);
}

void Target::parity_to(uint64_t parity_start_offset,
                       std::span<std::byte const> data,
                       std::span<std::byte> parity) noexcept {
  parity_start_offset = parity_start_offset % strip_sz_;
  data = data.subspan(
      0, std::min(stripe_data_sz_ - parity_start_offset, data.size()));
  parity = parity.subspan(0, std::min(strip_sz_, parity.size()));

  assert(is_multiple_of(parity_start_offset, sizeof(uint64_t)));
  assert(is_multiple_of(data.size(), sizeof(uint64_t)));
  assert(is_multiple_of(parity.size(), sizeof(uint64_t)));
  assert(!(parity.size() < strip_sz_));

  auto data_u64 = to_span_of<uint64_t const>(data);
  auto const parity_u64 = to_span_of<uint64_t>(parity);

  if (auto const parity_u64_offset = parity_start_offset / sizeof(uint64_t)) {
    auto const chunk_u64_len =
        std::min(data_u64.size(), parity_u64.size() - parity_u64_offset);
    auto const parity_u64_chunk =
        parity_u64.subspan(parity_u64_offset, chunk_u64_len);
    auto const data_u64_chunk = data_u64.subspan(0, chunk_u64_len);
    math::xor_to(data_u64_chunk, parity_u64_chunk);
    data_u64 = data_u64.subspan(data_u64_chunk.size());
  }

  for (; !(data_u64.size() < parity_u64.size());
       data_u64 = data_u64.subspan(parity_u64.size())) {
    math::xor_to(data_u64.subspan(0, parity_u64.size()), parity_u64);
  }

  if (!data_u64.empty())
    math::xor_to(data_u64, parity_u64.subspan(0, data_u64.size()));
}

void Target::parity_to(std::span<std::byte const> stripe_data,
                       std::span<std::byte> parity) noexcept {
  parity_to(0, stripe_data, parity);
}

void Target::parity_renew(std::span<std::byte const> stripe_data,
                          std::span<std::byte> parity) noexcept {
  algo::fill(to_span_of<uint64_t>(parity), UINT64_C(0));
  parity_to(stripe_data, parity);
}

std::vector<std::shared_ptr<IRWHandler>>
Target::stripe_id_to_handlers(uint64_t stripe_id) {
  std::vector<std::shared_ptr<IRWHandler>> hs;
  hs.reserve(hs_.size());

  auto const parity_id = stripe_id_to_parity_id(stripe_id);
  assert(parity_id < hs_.size());

  std::copy_n(hs_.begin(), parity_id, std::back_inserter(hs));
  std::rotate_copy(hs_.begin() + parity_id, hs_.begin() + parity_id + 1,
                   hs_.end(), std::back_inserter(hs));

  return hs;
}

ssize_t Target::stripe_write(uint64_t stripe_id_at, uint64_t stripe_offset,
                             std::span<std::byte const> data,
                             std::span<std::byte const> parity) noexcept {
  ssize_t wb{0};

  stripe_id_at += stripe_offset / stripe_data_sz_;
  stripe_offset = stripe_offset % stripe_data_sz_;
  data =
      data.subspan(0, std::min(stripe_data_sz_ - stripe_offset, data.size()));
  parity = parity.subspan(0, std::min(strip_sz_, parity.size()));

  assert(!(parity.size() < strip_sz_));

  auto hs = stripe_id_to_handlers(stripe_id_at);

  for (size_t hid = stripe_offset / strip_sz_,
              strip_offset = stripe_offset % strip_sz_;
       !data.empty() && hid < (hs.size() - 1); ++hid, strip_offset = 0) {
    auto const h_offset = stripe_id_at * strip_sz_ + strip_offset;
    auto const h_chunk =
        data.subspan(0, std::min(strip_sz_ - strip_offset, data.size()));
    if (auto const res = hs[hid]->write(h_chunk, h_offset); res >= 0)
        [[likely]] {
      assert(static_cast<size_t>(res) == h_chunk.size());
      wb += res;
    } else {
      return res;
    }
    data = data.subspan(h_chunk.size());
  }

  assert(data.empty());

  if (auto const res = hs.back()->write(parity, stripe_id_at * strip_sz_);
      res >= 0) [[likely]] {
    wb += res;
  } else {
    return res;
  }

  if (!(stripe_id_at < stripe_parity_coherency_state_.size())) [[unlikely]]
    stripe_parity_coherency_state_.resize(stripe_id_at + 1);
  stripe_parity_coherency_state_.set(stripe_id_at);

  return wb;
}

ssize_t Target::stripe_write(uint64_t stripe_id_at,
                             std::span<std::byte const> stripe) noexcept {
  assert(!(stripe.size() < (stripe_data_sz_ + strip_sz_)));
  auto const stripe_data = stripe.subspan(0, stripe_data_sz_);
  auto const parity = stripe.subspan(stripe_data_sz_);
  return stripe_write(stripe_id_at, 0, stripe_data, parity);
}

ssize_t Target::read(std::span<std::byte> buf, __off64_t offset) noexcept {
  return read_data_skip_parity(offset / stripe_data_sz_,
                               offset % stripe_data_sz_, buf);
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

    auto write_from = chunk;

    /*
     * Read the whole stripe from the backend excluding parity in case we intend
     * to partially modify the stripe
     */
    if (chunk.size() < cached_stripe_data_view().size()) {
      if (!(stripe_id < stripe_parity_coherency_state_.size())) [[unlikely]]
        stripe_parity_coherency_state_.resize(stripe_id + 1);

      if (stripe_parity_coherency_state_[stripe_id]) [[likely]] {
        auto const tmp_data_chunk =
            cached_stripe_data_view().subspan(stripe_offset, chunk.size());
        if (auto const res =
                read_stripe_data(stripe_id, stripe_offset, tmp_data_chunk);
            res < 0) [[unlikely]] {
          return res;
        } else if (auto const res = read_stripe_parity(
                       stripe_id, cached_stripe_parity_view());
                   res < 0) [[unlikely]] {
          return res;
        } else {
          /*
           * Renew a required chunk of parity of the stripe by computing a piece
           * of parity from the scratch basing on the result of old data being
           * XORed with a new data come in
           */
          math::xor_to(chunk, tmp_data_chunk);
          parity_to(stripe_offset % cached_stripe_parity_view().size(),
                    tmp_data_chunk, cached_stripe_parity_view());
        }
      } else if (auto const res =
                     read_stripe_data(stripe_id, cached_stripe_data_view());
                 res < 0) [[unlikely]] {
        return res;
      } else {
        auto const copy_from = chunk;
        auto const copy_to =
            cached_stripe_data_view().subspan(stripe_offset, chunk.size());

        /* Modify the part of the stripe with the new data come in */
        algo::copy(copy_from, copy_to);

        /* Renew Parity of the stripe */
        parity_renew(cached_stripe_data_view(), cached_stripe_parity_view());

        write_from = cached_stripe_data_view();
      }
      /*
       * Calculate parity based on newly incoming stripe-long chunk and write
       * back the whole stripe including the chunk and parity computed
       */
    } else {
      /* Renew Parity of the stripe */
      parity_renew(std::as_bytes(chunk), cached_stripe_parity_view());
    }

    /*
     * Write Back the chunk including the newly incoming data and the parity
     * computed and updated
     */
    if (auto const res =
            stripe_write(stripe_id, stripe_offset, std::as_bytes(write_from),
                         cached_stripe_parity_view());
        res < 0) [[unlikely]] {
      return res;
    }

    ++stripe_id;
    stripe_offset = 0;
    buf = buf.subspan(chunk.size());
    wb += chunk.size();
  }

  return wb;
}

} // namespace ublk::raidsp
