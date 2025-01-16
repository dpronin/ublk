#pragma once

#include <cstddef>
#include <cstdint>

#include <bit>
#include <concepts>

#include <gsl/assert>

#include "concepts.hpp"

namespace ublk {

constexpr bool is_power_of_2(std::unsigned_integral auto x) noexcept {
  return std::has_single_bit(x);
}

constexpr auto div_round_up(std::unsigned_integral auto x,
                            std::unsigned_integral auto y) noexcept {
  Expects(0 != y);
  return (x + y - 1) / y;
}

constexpr auto alignup(std::integral auto x, std::integral auto y) noexcept {
  return div_round_up(x, y) * y;
}

constexpr auto aligndown(std::integral auto x, std::integral auto y) noexcept {
  Expects(0 != y);
  return (x / y) * y;
}

/* 'y' must be a power of 2 */
constexpr auto align_up(std::integral auto x, std::integral auto y) noexcept {
  Expects(is_power_of_2(y));
  return ((x - 1) | (y - 1)) + 1;
}

/* 'y' must be a power of 2 */
constexpr auto align_down(std::integral auto x, std::integral auto y) noexcept {
  Expects(is_power_of_2(y));
  return x & ~(y - 1);
}

/* 'y' must be a power of 2 */
constexpr bool is_multiple_of(std::unsigned_integral auto x,
                              std::unsigned_integral auto y) noexcept {
  Expects(is_power_of_2(y));
  return 0 == (x & (y - 1));
}

/* 'y' must be a power of 2 */
constexpr bool is_aligned_to(std::integral auto x,
                             std::integral auto y) noexcept {
  return is_multiple_of(x, y);
}

template <standard_layout T, typename M>
constexpr ptrdiff_t offset_of(M T::*member) noexcept {
  return reinterpret_cast<ptrdiff_t>(&(reinterpret_cast<T *>(0)->*member));
}

template <standard_layout T, typename M1, typename M2 = M1>
constexpr T *container_of(M1 *ptr, M2 T::*member) noexcept {
  return reinterpret_cast<T *>(reinterpret_cast<uintptr_t>(ptr) -
                               offset_of(member));
}

/* helper type for std::visit */
template <class... Ts> struct overloaded : Ts... {
  using Ts::operator()...;
};

} // namespace ublk
