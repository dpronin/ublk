#pragma once

#include <cassert>

#include <memory>
#include <utility>

#include "read_handler_interface.hpp"
#include "rw_handler_interface.hpp"
#include "write_handler_interface.hpp"

namespace ublk {

class RWHandler : public IRWHandler {
public:
  explicit RWHandler(std::shared_ptr<IReadHandler> rh,
                     std::shared_ptr<IWriteHandler> wh)
      : rh_(std::move(rh)), wh_(std::move(wh)) {
    assert(rh_);
    assert(wh_);
  }
  ~RWHandler() override = default;

  RWHandler(RWHandler const &) = default;
  RWHandler &operator=(RWHandler const &) = default;

  RWHandler(RWHandler &&) = default;
  RWHandler &operator=(RWHandler &&) = default;

  ssize_t read(std::span<std::byte> buf, __off64_t offset) noexcept override {
    return rh_->handle(buf, offset);
  }

  ssize_t write(std::span<std::byte const> buf,
                __off64_t offset) noexcept override {
    return wh_->handle(buf, offset);
  }

private:
  std::shared_ptr<IReadHandler> rh_;
  std::shared_ptr<IWriteHandler> wh_;
};

} // namespace ublk
