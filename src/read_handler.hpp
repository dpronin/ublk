#pragma once

#include <unistd.h>

#include <cassert>

#include <memory>
#include <utility>

#include "read_handler_interface.hpp"

namespace cfq {

class ReadHandler : public IReadHandler {
public:
  explicit ReadHandler(std::shared_ptr<const int> fd) : fd_(std::move(fd)) {
    assert(fd_);
  }
  ~ReadHandler() override = default;

  ReadHandler(ReadHandler const &) = default;
  ReadHandler &operator=(ReadHandler const &) = default;

  ReadHandler(ReadHandler &&) = default;
  ReadHandler &operator=(ReadHandler &&) = default;

  ssize_t handle(std::span<std::byte> buf, __off64_t offset) noexcept override {
    if (auto const res = ::pread64(*fd_, buf.data(), buf.size(), offset);
        res > 0) [[likely]] {
      return res;
    } else if ((0 == res && ::ftruncate64(*fd_, offset + buf.size()) < 0) ||
               res < 0) {
      return -(errno < 0 ? errno : EIO);
    }
    return 0;
  }

private:
  std::shared_ptr<const int> fd_;
};

} // namespace cfq
