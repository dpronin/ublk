#pragma once

#include "cmd_interface.hpp"

namespace ublk::cli::cmds {
class Null final : public ICmd {
public:
  void exec() override {}
};
} // namespace ublk::cli::cmds
