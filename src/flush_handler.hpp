#pragma once

#include <unistd.h>

#include <cassert>

#include <memory>
#include <utility>

#include "flush_handler_interface.hpp"

namespace cfq {

class FlushHandler : public IFlushHandler {
public:
  explicit FlushHandler(std::shared_ptr<const int> fd) : fd_(std::move(fd)) {
    assert(fd_);
  }
  ~FlushHandler() override = default;

  FlushHandler(FlushHandler const &) = default;
  FlushHandler &operator=(FlushHandler const &) = default;

  FlushHandler(FlushHandler &&) = default;
  FlushHandler &operator=(FlushHandler &&) = default;

  int handle() noexcept override { return ::fsync(*fd_); }

private:
  std::shared_ptr<const int> fd_;
};

} // namespace cfq
