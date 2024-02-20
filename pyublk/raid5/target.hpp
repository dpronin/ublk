#pragma once

#include <cstddef>
#include <cstdint>

#include <memory>
#include <vector>

#include "mem_types.hpp"
#include "rw_handler_interface.hpp"
#include "sector.hpp"
#include "span.hpp"

namespace ublk::raid5 {

class Target {
private:
  constexpr static inline auto kCachedStripeAlignment = kSectorSz;
  static_assert(is_aligned_to(kCachedStripeAlignment,
                              alignof(std::max_align_t)));

  template <typename T = std::byte> auto cached_stripe_view() const noexcept {
    return to_span_of<T>({cached_stripe_.get(), stripe_data_sz_ + strip_sz_});
  }

  ssize_t cached_stripe_write(uint64_t stripe_id_at) noexcept;
  void cached_stripe_parity_renew(uint64_t parity_strip_id) noexcept;

protected:
  void parity_renew(uint64_t parity_strip_id,
                    std::span<std::byte> stripe) noexcept;
  ssize_t stripe_write(uint64_t stripe_id_at,
                       std::span<std::byte const> stripe) noexcept;
  ssize_t read_data_skip_parity(uint64_t strip_id_from,
                                uint64_t strip_offset_from,
                                std::span<std::byte> buf) noexcept;

public:
  explicit Target(uint64_t strip_sz,
                  std::vector<std::shared_ptr<IRWHandler>> hs);
  virtual ~Target() = default;

  Target(Target const &) = delete;
  Target &operator=(Target const &) = delete;

  Target(Target &&) = default;
  Target &operator=(Target &&) = default;

  virtual ssize_t read(std::span<std::byte> buf, __off64_t offset) noexcept;
  virtual ssize_t write(std::span<std::byte const> buf,
                        __off64_t offset) noexcept;

protected:
  uint64_t strip_sz_;
  uint64_t stripe_data_sz_;
  std::vector<std::shared_ptr<IRWHandler>> hs_;

private:
  uptrwd<std::byte[]> cached_stripe_;
};

} // namespace ublk::raid5