#pragma once

#include <cassert>
#include <cstddef>

#include <memory>
#include <utility>

#include <linux/ublkdrv/cmd.h>

#include "discard_handler_interface.hpp"
#include "handler_interface.hpp"

namespace ublk {

class CmdDiscardHandler : public IHandler<int(ublkdrv_cmd_discard) noexcept> {
public:
  explicit CmdDiscardHandler(std::shared_ptr<IDiscardHandler> discarder)
      : discarder_(std::move(discarder)) {
    assert(discarder_);
  }
  ~CmdDiscardHandler() override = default;

  CmdDiscardHandler(CmdDiscardHandler const &) = default;
  CmdDiscardHandler &operator=(CmdDiscardHandler const &) = default;

  CmdDiscardHandler(CmdDiscardHandler &&) = default;
  CmdDiscardHandler &operator=(CmdDiscardHandler &&) = default;

  int handle(ublkdrv_cmd_discard cmd) noexcept override {
    discarder_->handle(ublkdrv_cmd_discard_get_offset(&cmd),
                       ublkdrv_cmd_discard_get_sz(&cmd));
    return 0;
  }

private:
  std::shared_ptr<IDiscardHandler> discarder_;
};

} // namespace ublk
