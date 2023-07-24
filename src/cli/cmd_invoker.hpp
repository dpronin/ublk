#pragma once

#include <cassert>

#include <memory>
#include <utility>

#include "cli_ctx.hpp"

namespace ublk::cli {

class CmdInvoker final {
public:
  explicit CmdInvoker(std::shared_ptr<CliCtx> ctx) : ctx_(std::move(ctx)) {
    assert(ctx_);
  }

  void operator()(std::shared_ptr<ICmd> cmd) {
    assert(cmd);
    cmd->exec();
  }

private:
  std::shared_ptr<CliCtx> ctx_;
};

} // namespace ublk::cli
