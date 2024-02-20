#pragma once

#include <unistd.h>

#include <cassert>

#include <memory>
#include <utility>

#include "flush_handler_interface.hpp"
#include "target.hpp"

namespace ublk::def {

class FlushHandler : public IFlushHandler {
public:
  explicit FlushHandler(std::shared_ptr<Target> target)
      : target_(std::move(target)) {
    assert(target_);
  }
  ~FlushHandler() override = default;

  FlushHandler(FlushHandler const &) = default;
  FlushHandler &operator=(FlushHandler const &) = default;

  FlushHandler(FlushHandler &&) = default;
  FlushHandler &operator=(FlushHandler &&) = default;

  int handle() noexcept override { return target_->fsync(); }

private:
  std::shared_ptr<Target> target_;
};

} // namespace ublk::def