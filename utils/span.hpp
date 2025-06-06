#pragma once

#include <cstddef>
#include <cstdint>

#include <concepts>
#include <span>
#include <type_traits>

#include <gsl/assert>

#include "utility.hpp"

namespace ublk {

template <typename T, typename S = std::byte>
  requires std::disjunction<
      std::is_unsigned<T>,
      std::is_same<std::remove_const_t<T>, std::byte>>::value
auto to_span_of(std::span<S> data) noexcept {
  if constexpr (std::same_as<T, S>) {
    return data;
  } else {
    Expects(
        is_multiple_of(reinterpret_cast<uintptr_t>(data.data()), sizeof(T)));
    Expects(is_multiple_of(data.size(), sizeof(T)));
    return std::span{
        reinterpret_cast<T *>(data.data()),
        reinterpret_cast<T *>(data.data() + data.size()),
    };
  }
}

template <typename T> std::span<T> const_span_cast(std::span<T const> from) {
  return std::span{const_cast<T *>(from.data()), from.size()};
}

} // namespace ublk
