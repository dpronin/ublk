#pragma once

#include <sys/epoll.h>

#include <system_error>

#include "file.hpp"
#include "mem_types.hpp"

namespace ublk {

template <typename... Args> uptrwd<int const> epoll_create1(Args... args) {
  int err{0};
  if (auto const fd = ::epoll_create1(args...); fd >= 0) {
    if (auto result =
            uptrwd<int const>{new (std::nothrow) int{fd}, detail::pfddcloser})
      return result;
    err = ENOMEM;
    close(fd);
  } else {
    err = errno;
  }
  throw std::system_error(err, std::generic_category());
}

} // namespace ublk
