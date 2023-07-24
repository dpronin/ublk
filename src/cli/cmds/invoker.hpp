#pragma once

#include <cassert>

#include <memory>

#include "cli/cmds/cmd_interface.hpp"

namespace ublk::cli::cmds {

class Invoker final {
public:
  void operator()(std::shared_ptr<ICmd> cmd) {
    assert(cmd);
    cmd->exec();
  }
};

} // namespace ublk::cli::cmds
