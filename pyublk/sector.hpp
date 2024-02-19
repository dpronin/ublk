#pragma once

#include <cstdint>

#include <concepts>

namespace ublk {

constexpr inline uint64_t kSectorShift = 9;
constexpr inline uint64_t kSectorSz = 1 << kSectorShift;

auto sectors_to_bytes(std::unsigned_integral auto x) noexcept {
  return x << kSectorShift;
}

auto bytes_to_sectors(std::unsigned_integral auto x) noexcept {
  return x >> kSectorShift;
}

} // namespace ublk
