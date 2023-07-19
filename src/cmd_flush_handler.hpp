#pragma once

#include <cassert>
#include <cstddef>

#include <memory>

#include <linux/ublk/cmd.h>

#include "cmd_handler_interface.hpp"
#include "flush_handler_interface.hpp"

namespace cfq {

class CmdFlushHandler : public ICmdHandler<const ublk_cmd_flush> {
public:
  explicit CmdFlushHandler(std::shared_ptr<IFlushHandler> flusher)
      : flusher_(std::move(flusher)) {
    assert(flusher_);
  }
  ~CmdFlushHandler() override = default;

  CmdFlushHandler(CmdFlushHandler const &) = delete;
  CmdFlushHandler &operator=(CmdFlushHandler const &) = delete;

  CmdFlushHandler(CmdFlushHandler &&) = default;
  CmdFlushHandler &operator=(CmdFlushHandler &&) = default;

  int handle(ublk_cmd_flush const &cmd [[maybe_unused]]) noexcept override {
    return flusher_->handle();
  }

private:
  std::shared_ptr<IFlushHandler> flusher_;
};

} // namespace cfq
