#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>

#include <concepts>
#include <span>
#include <type_traits>

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
    assert(is_multiple_of(reinterpret_cast<uintptr_t>(data.data()), sizeof(T)));
    assert(is_multiple_of(data.size(), sizeof(T)));
    return std::span{
        reinterpret_cast<T *>(data.data()),
        reinterpret_cast<T *>(data.data() + data.size()),
    };
  }
}

} // namespace ublk
