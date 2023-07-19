#pragma once

#include <cstddef>
#include <cstdint>

#include <algorithm>
#include <concepts>
#include <ranges>

#include "boost/algorithm/string/trim_all.hpp"

#include "types.hpp"

namespace cfq {

inline auto normalize(std::ranges::range auto range) {
  // trim all the possible whitespace symbols in arguments
  for (auto &item : range)
    boost::algorithm::trim_all(item);
  // erase all the arguments that are empty strings
  std::erase_if(range, [](auto const &arg) { return arg.empty(); });
  return range;
}

template <typename T, typename M>
constexpr ptrdiff_t offset_of(M const T::*member) {
  return reinterpret_cast<ptrdiff_t>(&(reinterpret_cast<T *>(0)->*member));
}

template <typename T, typename M>
constexpr T *container_of(M const *ptr, M const T::*member) {
  return reinterpret_cast<T *>(reinterpret_cast<uintptr_t>(ptr) -
                               offset_of(member));
}

} // namespace cfq
