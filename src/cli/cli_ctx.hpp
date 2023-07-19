#pragma once

#include <cassert>

#include <algorithm>
#include <iterator>
#include <memory>
#include <ranges>
#include <sstream>
#include <utility>

#include "cli_state.hpp"
#include "cmd_parser_interface.hpp"
#include "cmd_state.hpp"
#include "readline.hpp"

namespace cfq::cli {

class CliCtx final {
public:
  void set_state(std::unique_ptr<CmdState> state) {
    assert(state);
    state_ = std::move(state);
    rl_.set_suggester(state_);
  }

  explicit CliCtx(Readline &rl, std::shared_ptr<bool> finish_token,
                  std::unique_ptr<CmdState> init_state)
      : rl_(rl), finish_token_(std::move(finish_token)) {
    assert(finish_token_);
    set_state(std::move(init_state));
  }
  ~CliCtx() = default;

  CliCtx(CliCtx const &) = delete;
  CliCtx &operator=(CliCtx const &) = delete;

  CliCtx(CliCtx &&) = delete;
  CliCtx &operator=(CliCtx &&) = delete;

  [[nodiscard]] auto ureadcmd() {
    auto const to_args = [](user_input_t const &user_input) {
      args_t args;
      std::istringstream iss{user_input};
      std::ranges::copy(std::views::istream<arg_t>(iss),
                        std::back_inserter(args));
      return args;
    };
    return state_->parser().parse(to_args(rl_(state_->prompt())));
  }

  [[nodiscard]] explicit operator bool() const noexcept {
    return !(*finish_token_);
  }

private:
  Readline &rl_;
  std::shared_ptr<bool> finish_token_;

  std::shared_ptr<CmdState> state_;
};

} // namespace cfq::cli
