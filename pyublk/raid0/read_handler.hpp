#pragma once

#include <cassert>

#include <memory>
#include <utility>

#include "read_handler_interface.hpp"
#include "target.hpp"

namespace ublk::raid0 {

class ReadHandler : public IReadHandler {
public:
  explicit ReadHandler(std::shared_ptr<Target> target)
      : target_(std::move(target)) {
    assert(target_);
  }
  ~ReadHandler() override = default;

  ReadHandler(ReadHandler const &) = default;
  ReadHandler &operator=(ReadHandler const &) = default;

  ReadHandler(ReadHandler &&) = default;
  ReadHandler &operator=(ReadHandler &&) = default;

  ssize_t handle(std::span<std::byte> buf, __off64_t offset) noexcept override {
    return target_->read(buf, offset);
  }

private:
  std::shared_ptr<Target> target_;
};

} // namespace ublk::raid0
