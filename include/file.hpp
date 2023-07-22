#pragma once

#include <fcntl.h>
#include <unistd.h>

#include <filesystem>
#include <fstream>
#include <system_error>

#include "mem.hpp"

namespace cfq {

namespace detail {

constexpr auto pfdcloser = +[](int const *pfd) { close(*pfd); };
constexpr auto pfddcloser = +[](int const *pfd) {
  pfdcloser(pfd);
  delete pfd;
};

} // namespace detail

template <typename... Args>
uptrwd<int const> open(std::filesystem::path const &path, Args... args) {
  if (auto const fd = ::open(path.c_str(), args...); fd >= 0)
    return {new int{fd}, detail::pfddcloser};
  throw std::system_error(errno, std::generic_category());
}

template <typename Target, typename... IfStreamModifiers>
Target read_from_as(std::filesystem::path const &attr_path,
                    IfStreamModifiers &&...modifiers) {
  Target v;
  std::ifstream f{attr_path};
  ((f >> std::forward<IfStreamModifiers>(modifiers)), ...);
  f >> v;
  return v;
}

} // namespace cfq
