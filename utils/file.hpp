#pragma once

#include <fcntl.h>
#include <unistd.h>

#include <filesystem>
#include <fstream>
#include <new>
#include <system_error>

#include "mm/mem_types.hpp"

namespace ublk {

namespace detail {

constexpr auto pfdcloser = +[](int const *pfd) { close(*pfd); };
constexpr auto pfddcloser = +[](int const *pfd) {
  pfdcloser(pfd);
  delete pfd;
};

} // namespace detail

template <typename... Args>
mm::uptrwd<int const> open(std::filesystem::path const &path, Args... args) {
  int err{0};
  if (auto const fd = ::open(path.c_str(), args...); fd >= 0) {
    if (auto result = mm::uptrwd<int const>{new (std::nothrow) int{fd},
                                            detail::pfddcloser})
      return result;
    err = ENOMEM;
    close(fd);
  } else {
    err = errno;
  }
  throw std::system_error(err, std::generic_category());
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

} // namespace ublk
