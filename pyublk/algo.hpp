#pragma once

#include <cstdint>

#include <algorithm>
#include <concepts>
#include <iterator>
#include <span>
#include <type_traits>

#include <eve/module/algo.hpp>

#include "span.hpp"

#include "concepts.hpp"

namespace ublk::algo {

namespace detail {

template <typename T>
  requires std::integral<T> || is_byte<T>
void copy_stl(std::span<T const> from, std::span<T> to) noexcept {
  std::ranges::copy(from, std::begin(to));
}

template <typename T>
  requires std::integral<T> || is_byte<T>
void copy_eve(std::span<T const> from, std::span<T> to) noexcept {
  if constexpr (std::is_same_v<T, std::byte>)
    eve::algo::copy(to_span_of<uint8_t const>(from), to_span_of<uint8_t>(to));
  else
    eve::algo::copy(from, to);
}

template <typename T>
  requires std::integral<T> || is_byte<T>
void fill_stl(std::span<T> range, T value) noexcept {
  std::ranges::fill(range, value);
}

template <typename T>
  requires std::integral<T> || is_byte<T>
void fill_eve(std::span<T> range, T value) noexcept {
  if constexpr (std::is_same_v<T, std::byte>)
    eve::algo::fill(to_span_of<uint8_t>(range), static_cast<uint8_t>(value));
  else
    eve::algo::fill(range, value);
}

} // namespace detail

template <typename T>
  requires std::integral<T> || is_byte<T>
void copy(std::span<T const> from, std::span<T> to) noexcept {
#ifndef NDEBUG
  detail::copy_stl(from, to);
#else
  detail::copy_stl(from, to);
  /*
   * NOTE(dannftk@yandex.ru): eve always works worse than runtime in release
   * with gcc and clang
   */
  // detail::copy_eve(from, to);
#endif
}

template <typename T>
  requires std::integral<T> || is_byte<T>
void fill(std::span<T> range, T value) noexcept {
#ifndef NDEBUG
  if constexpr (sizeof(T) > 1)
    return detail::fill_stl(range, value);
  else
    return detail::fill_eve(range, value);
#else
  return detail::fill_stl(range, value);
#endif
}

} // namespace ublk::algo
