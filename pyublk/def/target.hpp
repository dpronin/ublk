#pragma once

#include <cassert>
#include <cerrno>
#include <cstddef>

#include <unistd.h>

#include <bits/types.h>
#include <sys/types.h>

#include <memory>
#include <span>
#include <utility>

#include "mem_types.hpp"

namespace ublk::def {

class Target {
public:
  explicit Target(uptrwd<const int> fd) : fd_(std::move(fd)) { assert(fd_); }
  virtual ~Target() = default;

  Target(Target const &) = delete;
  Target &operator=(Target const &) = delete;

  Target(Target &&) = default;
  Target &operator=(Target &&) = default;

  virtual ssize_t read(std::span<std::byte> buf, __off64_t offset) noexcept {
    ssize_t rb{0};

    while (!buf.empty()) {
      if (auto const res = ::pread64(*fd_, buf.data(), buf.size(), offset);
          res > 0) [[likely]] {
        rb += res;
        offset += res;
        buf = buf.subspan(res);
      } else if ((0 == res && ::ftruncate64(*fd_, offset + buf.size()) < 0) ||
                 res < 0) {
        return -(errno ?: EIO);
      }
    }

    return rb;
  }

  virtual ssize_t write(std::span<std::byte const> buf,
                        __off64_t offset) noexcept {
    if (auto const res = ::pwrite64(*fd_, buf.data(), buf.size(), offset);
        res >= 0) [[likely]] {
      return res;
    } else {
      return -(errno ?: EIO);
    }
  }

  virtual int fsync() noexcept { return ::fsync(*fd_); }

protected:
  uptrwd<const int> fd_;
};

} // namespace ublk::def
