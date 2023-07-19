#pragma once

#include <fcntl.h>
#include <system_error>
#include <unistd.h>

#include <filesystem>

#include "mem.hpp"

namespace cfq {

constexpr auto pfdcloser = +[](int const *pfd) { close(*pfd); };

template <typename... Args>
uptrwd<int const> open(std::filesystem::path const &path, Args... args) {
  if (auto const fd = ::open(path.c_str(), args...); fd >= 0) {
    return {
        new int{fd},
        [](auto const *pfd) {
          pfdcloser(pfd);
          delete pfd;
        },
    };
  }
  throw std::system_error(errno, std::generic_category());
}

} // namespace cfq
