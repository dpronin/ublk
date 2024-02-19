#pragma once

#include <cstdint>

#include <algorithm>
#include <concepts>
#include <functional>
#include <iterator>
#include <span>
#include <type_traits>

#include <eve/module/algo.hpp>
#include <eve/module/math.hpp>

#include "span.hpp"

#include "concepts.hpp"

namespace ublk::math {

namespace detail {

template <typename T>
  requires std::integral<T> || is_byte<T>
auto xor_to_stl(std::span<T const> in, std::span<T> inout) noexcept {
  auto const data_src = in;
  auto const data_dst = inout;

  std::ranges::transform(data_src, data_dst, data_dst.begin(),
                         std::bit_xor<>{});

  return inout;
}

template <typename T>
  requires std::integral<T> || is_byte<T>
auto xor_to_eve(std::span<T const> in, std::span<T> inout) noexcept {
  using U = std::conditional_t<is_byte<T>, uint8_t, T>;
  using UC = std::add_const_t<U>;

  auto const data_src = to_span_of<UC>(in);
  auto const data_dst = to_span_of<U>(inout);
  eve::algo::transform_to(eve::views::zip(data_src, data_dst), data_dst,
                          eve::bit_xor);

  return inout;
}

} // namespace detail

template <typename T>
  requires std::integral<T> || is_byte<T>
auto xor_to(std::span<T const> in, std::span<T> inout) noexcept {
#ifndef NDEBUG
#ifdef __clang__
  if constexpr (sizeof(T) > 1)
    return detail::xor_to_stl(in, inout);
  else
    return detail::xor_to_eve(in, inout);
#else /* GNU GCC is assumed */
  if constexpr (!(sizeof(T) < 8))
    return detail::xor_to_stl(in, inout);
  else
    return detail::xor_to_eve(in, inout);
#endif
#else
  return detail::xor_to_eve(in, inout);
#endif
}

} // namespace ublk::math
