#pragma once

#include <memory>
#include <ranges>
#include <utility>

#include "cli_ctx.hpp"
#include "cmd_help.hpp"
#include "cmd_interface.hpp"
#include "cmd_parser_default.hpp"
#include "cmd_quit.hpp"

#include "color.hpp"
#include "types.hpp"

#include "cmd_state.hpp"

namespace cfq {

class CmdInvoker final {
public:
  explicit CmdInvoker(std::shared_ptr<CliCtx> ctx) : ctx_(std::move(ctx)) {
    assert(ctx_);
  }

  void operator()(std::shared_ptr<ICmd> cmd) {
    if (cmd)
      cmd->exec();
  }

  explicit operator bool() const noexcept { return static_cast<bool>(*ctx_); }

private:
  std::shared_ptr<CliCtx> ctx_;
};

} // namespace cfq
