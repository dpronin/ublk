#pragma once

#include <sys/epoll.h>

#include <system_error>

#include "file.hpp"
#include "mem.hpp"

namespace ublk {

template <typename... Args> uptrwd<int const> epoll_create1(Args... args) {
  if (auto const fd = ::epoll_create1(args...); fd >= 0)
    return {new int{fd}, detail::pfddcloser};
  throw std::system_error(errno, std::generic_category());
}

} // namespace ublk
