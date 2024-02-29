#pragma once

#include <cstddef>

#include <span>

namespace ublk {

void parity_to(std::span<std::byte const> data, size_t parity_start_offset,
               std::span<std::byte> parity) noexcept;

inline void parity_to(std::span<std::byte const> data,
                      std::span<std::byte> parity) noexcept {
  parity_to(data, 0, parity);
}

void parity_renew(std::span<std::byte const> data,
                  std::span<std::byte> parity) noexcept;

} // namespace ublk
