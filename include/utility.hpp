#pragma once

#include <cstddef>
#include <cstdint>

#include <algorithm>
#include <concepts>
#include <ranges>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/constants.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim_all.hpp>

namespace ublk {

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

template <typename Target>
auto split_as(auto &&range, std::string_view separators = " ") {
  Target r;
  boost::split(r, std::forward<decltype(range)>(range),
               boost::is_any_of(separators), boost::token_compress_on);
  return r;
};

} // namespace ublk
