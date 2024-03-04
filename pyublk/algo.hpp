#pragma once

#include <cassert>
#include <cstdint>

#include <algorithm>
#include <concepts>
#include <iterator>
#include <span>
#include <type_traits>

#include <eve/module/algo.hpp>

#include "concepts.hpp"
#include "span.hpp"
#include "utility.hpp"

namespace ublk::algo {

namespace detail {

template <typename T>
  requires std::integral<T> || is_byte<T>
void copy_stl(std::span<T const> from, std::span<T> to) noexcept {
  assert(!(from.size() > to.size()));
  if constexpr (is_byte<T>) {
    if (is_aligned_to(reinterpret_cast<uintptr_t>(from.data()),
                      sizeof(uint64_t)) &&
        is_aligned_to(from.size(), sizeof(uint64_t)) &&
        is_aligned_to(reinterpret_cast<uintptr_t>(to.data()),
                      sizeof(uint64_t)) &&
        is_aligned_to(to.size(), sizeof(uint64_t))) {
      std::ranges::copy(to_span_of<uint64_t const>(from),
                        std::begin(to_span_of<uint64_t>(to)));
    } else if (is_aligned_to(reinterpret_cast<uintptr_t>(from.data()),
                             sizeof(uint32_t)) &&
               is_aligned_to(from.size(), sizeof(uint32_t)) &&
               is_aligned_to(reinterpret_cast<uintptr_t>(to.data()),
                             sizeof(uint32_t)) &&
               is_aligned_to(to.size(), sizeof(uint32_t))) {
      std::ranges::copy(to_span_of<uint32_t const>(from),
                        std::begin(to_span_of<uint32_t>(to)));
    } else if (is_aligned_to(reinterpret_cast<uintptr_t>(from.data()),
                             sizeof(uint16_t)) &&
               is_aligned_to(from.size(), sizeof(uint16_t)) &&
               is_aligned_to(reinterpret_cast<uintptr_t>(to.data()),
                             sizeof(uint16_t)) &&
               is_aligned_to(to.size(), sizeof(uint16_t))) {
      std::ranges::copy(to_span_of<uint16_t const>(from),
                        std::begin(to_span_of<uint16_t>(to)));
    } else {
      std::ranges::copy(from, std::begin(to));
    }
  } else {
    std::ranges::copy(from, std::begin(to));
  }
}

template <typename T>
  requires std::integral<T> || is_byte<T>
void copy_eve(std::span<T const> from, std::span<T> to) noexcept {
  assert(!(from.size() > to.size()));
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
