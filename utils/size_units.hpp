#pragma once

#include <cstddef>

namespace ublk {

inline namespace literals {

inline namespace size_units_literals {

constexpr auto operator"" _KiB(unsigned long long x) noexcept {
  return x << 10;
}

constexpr auto operator"" _MiB(unsigned long long x) noexcept {
  return x << 20;
}

constexpr auto operator"" _GiB(unsigned long long x) noexcept {
  return x << 30;
}

constexpr auto operator"" _TiB(unsigned long long x) noexcept {
  return x << 40;
}

constexpr auto operator"" _PiB(unsigned long long x) noexcept {
  return x << 50;
}

constexpr auto operator"" _EiB(unsigned long long x) noexcept {
  return x << 60;
}

constexpr auto operator"" _KB(unsigned long long x) noexcept {
  return x * 1000;
}

constexpr auto operator"" _MB(unsigned long long x) noexcept {
  return x * 1'000'000;
}

constexpr auto operator"" _GB(unsigned long long x) noexcept {
  return x * 1'000'000'000;
}

constexpr auto operator"" _TB(unsigned long long x) noexcept {
  return x * 1'000'000'000'000;
}

constexpr auto operator"" _PB(unsigned long long x) noexcept {
  return x * 1'000'000'000'000'000;
}

constexpr auto operator"" _EB(unsigned long long x) noexcept {
  return x * 1'000'000'000'000'000'000;
}

constexpr auto operator"" _sz(unsigned long long x) noexcept {
  return static_cast<std::size_t>(x);
}

constexpr auto operator"" _b(unsigned long long x) noexcept {
  return static_cast<std::byte>(x);
}

} // namespace size_units_literals

} // namespace literals

} // namespace ublk
