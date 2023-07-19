#pragma once

#include <unistd.h>

#include <cassert>

#include <memory>
#include <utility>

#include "write_handler_interface.hpp"

namespace cfq {

class WriteHandler : public IWriteHandler {
public:
  explicit WriteHandler(std::shared_ptr<const int> fd) : fd_(std::move(fd)) {
    assert(fd_);
  }
  ~WriteHandler() override = default;

  WriteHandler(WriteHandler const &) = default;
  WriteHandler &operator=(WriteHandler const &) = default;

  WriteHandler(WriteHandler &&) = default;
  WriteHandler &operator=(WriteHandler &&) = default;

  ssize_t handle(std::span<std::byte const> buf,
                 __off64_t offset) noexcept override {
    if (auto const res = ::pwrite64(*fd_, buf.data(), buf.size(), offset);
        res >= 0) [[likely]] {

      return res;
    } else {
      return -(errno < 0 ? errno : EIO);
    }
  }

private:
  std::shared_ptr<const int> fd_;
};

} // namespace cfq
