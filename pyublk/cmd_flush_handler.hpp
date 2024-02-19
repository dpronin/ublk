#pragma once

#include <cassert>
#include <cstddef>

#include <memory>
#include <utility>

#include <linux/ublkdrv/cmd.h>

#include "flush_handler_interface.hpp"
#include "handler_interface.hpp"

namespace ublk {

class CmdFlushHandler : public IHandler<int(ublkdrv_cmd_flush) noexcept> {
public:
  explicit CmdFlushHandler(std::shared_ptr<IFlushHandler> flusher)
      : flusher_(std::move(flusher)) {
    assert(flusher_);
  }
  ~CmdFlushHandler() override = default;

  CmdFlushHandler(CmdFlushHandler const &) = default;
  CmdFlushHandler &operator=(CmdFlushHandler const &) = default;

  CmdFlushHandler(CmdFlushHandler &&) = default;
  CmdFlushHandler &operator=(CmdFlushHandler &&) = default;

  int handle(ublkdrv_cmd_flush cmd [[maybe_unused]]) noexcept override {
    return flusher_->handle();
  }

private:
  std::shared_ptr<IFlushHandler> flusher_;
};

} // namespace ublk
