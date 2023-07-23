#pragma once

#include <cassert>

#include <algorithm>
#include <iterator>
#include <memory>
#include <utility>

#include "cli_state.hpp"
#include "cmd_parser_interface.hpp"
#include "cmd_state.hpp"
#include "readline.hpp"
#include "utility.hpp"

namespace ublk::cli {

class CliCtx final {
public:
  void set_state(std::unique_ptr<CmdState> state) {
    assert(state);
    state_ = std::move(state);
    rl_.set_suggester(state_);
  }

  explicit CliCtx(Readline &rl, std::unique_ptr<CmdState> init_state)
      : rl_(rl) {

    set_state(std::move(init_state));
  }
  ~CliCtx() = default;

  CliCtx(CliCtx const &) = delete;
  CliCtx &operator=(CliCtx const &) = delete;

  CliCtx(CliCtx &&) = delete;
  CliCtx &operator=(CliCtx &&) = delete;

  [[nodiscard]] auto ureadcmd() {
    return state_->parser().parse(split_as<args_t>(rl_(state_->prompt())));
  }

private:
  Readline &rl_;
  std::shared_ptr<CmdState> state_;
};

} // namespace ublk::cli
