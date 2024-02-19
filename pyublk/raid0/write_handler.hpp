#pragma once

#include <cassert>

#include <memory>
#include <utility>

#include "target.hpp"
#include "write_handler_interface.hpp"

namespace ublk::raid0 {

class WriteHandler : public IWriteHandler {
public:
  explicit WriteHandler(std::shared_ptr<Target> target)
      : target_(std::move(target)) {
    assert(target_);
  }
  ~WriteHandler() override = default;

  WriteHandler(WriteHandler const &) = default;
  WriteHandler &operator=(WriteHandler const &) = default;

  WriteHandler(WriteHandler &&) = default;
  WriteHandler &operator=(WriteHandler &&) = default;

  ssize_t handle(std::span<std::byte const> buf,
                 __off64_t offset) noexcept override {
    return target_->write(buf, offset);
  }

private:
  std::shared_ptr<Target> target_;
};

} // namespace ublk::raid0
