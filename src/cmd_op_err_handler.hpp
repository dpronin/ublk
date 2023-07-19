#pragma once

#include <linux/ublk/cmd.h>

#include "handler_interface.hpp"

namespace cfq {

template <int eVal>
class CmdOpErrHandler : public IHandler<int(ublk_cmd) noexcept> {
public:
  CmdOpErrHandler() = default;
  ~CmdOpErrHandler() override = default;

  CmdOpErrHandler(CmdOpErrHandler const &) = default;
  CmdOpErrHandler &operator=(CmdOpErrHandler const &) = default;

  CmdOpErrHandler(CmdOpErrHandler &&) = default;
  CmdOpErrHandler &operator=(CmdOpErrHandler &&) = default;

  int handle(ublk_cmd cmd [[maybe_unused]]) noexcept override {
    return eVal;
  }
};

} // namespace cfq
