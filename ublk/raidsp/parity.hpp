#pragma once

#include <cstddef>

#include <span>

namespace ublk {

void parity_to(std::span<std::byte const> data, std::span<std::byte> parity,
               size_t parity_start_offset = 0) noexcept;

void parity_renew(std::span<std::byte const> data,
                  std::span<std::byte> parity) noexcept;

} // namespace ublk
