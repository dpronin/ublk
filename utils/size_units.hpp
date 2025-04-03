#pragma once

#include <cstddef>

namespace ublk {

inline namespace literals {

inline namespace size_units_literals {

constexpr auto inline kKiBShift = 10u;

constexpr auto operator""_KiB(unsigned long long x) noexcept {
  return x << kKiBShift;
}

constexpr auto inline kMiBShift = 20u;

constexpr auto operator""_MiB(unsigned long long x) noexcept {
  return x << kMiBShift;
}

constexpr auto inline kGiBShift = 30u;

constexpr auto operator""_GiB(unsigned long long x) noexcept {
  return x << kGiBShift;
}

constexpr auto inline kTiBShift = 40u;

constexpr auto operator""_TiB(unsigned long long x) noexcept {
  return x << kTiBShift;
}

constexpr auto inline kPiBShift = 50u;

constexpr auto operator""_PiB(unsigned long long x) noexcept {
  return x << kPiBShift;
}

constexpr auto inline kEiBShift = 60u;

constexpr auto operator""_EiB(unsigned long long x) noexcept {
  return x << kEiBShift;
}

constexpr auto operator""_KB(unsigned long long x) noexcept {
  return x * 1000;
}

constexpr auto operator""_MB(unsigned long long x) noexcept {
  return x * 1'000'000;
}

constexpr auto operator""_GB(unsigned long long x) noexcept {
  return x * 1'000'000'000;
}

constexpr auto operator""_TB(unsigned long long x) noexcept {
  return x * 1'000'000'000'000;
}

constexpr auto operator""_PB(unsigned long long x) noexcept {
  return x * 1'000'000'000'000'000;
}

constexpr auto operator""_EB(unsigned long long x) noexcept {
  return x * 1'000'000'000'000'000'000;
}

constexpr auto operator""_b(unsigned long long x) noexcept {
  return static_cast<std::byte>(x);
}

} // namespace size_units_literals

} // namespace literals

} // namespace ublk
