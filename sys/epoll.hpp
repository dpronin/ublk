#pragma once

#include <sys/epoll.h>

#include <system_error>

#include "mm/mem_types.hpp"

#include "file.hpp"

namespace ublk::sys {

template <typename... Args> mm::uptrwd<int const> epoll_create1(Args... args) {
  int err{0};
  if (auto const fd = ::epoll_create1(args...); fd >= 0) {
    if (auto result = mm::uptrwd<int const>{
            new (std::nothrow) int{fd},
            detail::pfddcloser,
        })
      return result;
    err = ENOMEM;
    close(fd);
  } else {
    err = errno;
  }
  throw std::system_error(err, std::generic_category());
}

} // namespace ublk::sys
