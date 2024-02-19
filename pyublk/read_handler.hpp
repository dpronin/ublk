#pragma once

#include <cassert>

#include <memory>
#include <utility>

#include "read_handler_interface.hpp"
#include "rw_handler_interface.hpp"

namespace ublk {

class ReadHandler : public IReadHandler {
public:
  explicit ReadHandler(std::shared_ptr<IRWHandler> rwh) : rwh_(std::move(rwh)) {
    assert(rwh_);
  }
  ~ReadHandler() override = default;

  ReadHandler(ReadHandler const &) = default;
  ReadHandler &operator=(ReadHandler const &) = default;

  ReadHandler(ReadHandler &&) = default;
  ReadHandler &operator=(ReadHandler &&) = default;

  ssize_t handle(std::span<std::byte> buf, __off64_t offset) noexcept override {
    return rwh_->read(buf, offset);
  }

private:
  std::shared_ptr<IRWHandler> rwh_;
};

} // namespace ublk
