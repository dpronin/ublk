#pragma once

#include "cmd_interface.hpp"

namespace ublk::cli {
class CmdNull final : public ICmd {
public:
  void exec() override {}
};
} // namespace ublk::cli
