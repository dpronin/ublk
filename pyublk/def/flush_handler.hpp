#pragma once

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

  FlushHandler(FlushHandler const &) = delete;
  FlushHandler &operator=(FlushHandler const &) = delete;

  FlushHandler(FlushHandler &&) = delete;
  FlushHandler &operator=(FlushHandler &&) = delete;

  int submit(std::shared_ptr<flush_query> fq) noexcept override {
    return target_->process(std::move(fq));
  }

private:
  std::shared_ptr<Target> target_;
};

} // namespace ublk::def
