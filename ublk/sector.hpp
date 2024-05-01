#pragma once

#include <cstdint>

#include <concepts>

namespace ublk {

constexpr inline auto kSectorShift{UINT64_C(9)};
constexpr inline auto kSectorSz{UINT64_C(1) << kSectorShift};

constexpr auto sectors_to_bytes(std::unsigned_integral auto x) noexcept {
  return x << kSectorShift;
}

constexpr auto bytes_to_sectors(std::unsigned_integral auto x) noexcept {
  return x >> kSectorShift;
}

} // namespace ublk
