#pragma once

#include <linux/ublk/cmd.h>

#include "cmd_handler_interface.hpp"

namespace cfq {

template <int eVal> class CmdOpErrHandler : public ICmdHandler<const ublk_cmd> {
public:
  CmdOpErrHandler() = default;
  ~CmdOpErrHandler() override = default;

  CmdOpErrHandler(CmdOpErrHandler const &) = default;
  CmdOpErrHandler &operator=(CmdOpErrHandler const &) = default;

  CmdOpErrHandler(CmdOpErrHandler &&) = default;
  CmdOpErrHandler &operator=(CmdOpErrHandler &&) = default;

  int handle(ublk_cmd const &cmd [[maybe_unused]]) noexcept override {
    return eVal;
  }
};

} // namespace cfq
