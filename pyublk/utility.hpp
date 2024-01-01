#pragma once

#include <cstddef>
#include <cstdint>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/constants.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim_all.hpp>

namespace ublk {

template <typename T, typename M>
constexpr ptrdiff_t offset_of(M const T::*member) noexcept {
  return reinterpret_cast<ptrdiff_t>(&(reinterpret_cast<T *>(0)->*member));
}

template <typename T, typename M>
constexpr T *container_of(M const *ptr, M const T::*member) noexcept {
  return reinterpret_cast<T *>(reinterpret_cast<uintptr_t>(ptr) -
                               offset_of(member));
}

} // namespace ublk
