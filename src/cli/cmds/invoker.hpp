#pragma once

#include <cassert>

#include <memory>
#include <utility>

#include "cli/cli_ctx.hpp"
#include "cli/cmds/cmd_interface.hpp"

namespace ublk::cli::cmds {

class Invoker final {
public:
  explicit Invoker(std::shared_ptr<CliCtx> ctx) : ctx_(std::move(ctx)) {
    assert(ctx_);
  }

  void operator()(std::shared_ptr<ICmd> cmd) {
    assert(cmd);
    cmd->exec();
  }

private:
  std::shared_ptr<CliCtx> ctx_;
};

} // namespace ublk::cli::cmds
