#pragma once

#include <algorithm>
#include <ranges>
#include <string_view>
#include <utility>

#include "cli/cmds/help.hpp"
#include "cli/cmds/parser_default.hpp"
#include "cli_state.hpp"
#include "color.hpp"
#include "types.hpp"

namespace ublk::cli {

class ShellState : public CliState {
private:
  suggestions_t to_suggestions(cmds_t const &cmds) {
    suggestions_t cmds_names{cmds.size()};
    auto named = cmds | std::views::transform(
                            [](auto const &cmd) { return std::get<0>(cmd); });
    std::ranges::copy(named, std::begin(cmds_names));
    return cmds_names;
  }

private:
  explicit ShellState(int, cmds_t cmds = {}, std::string_view prompt = "")
      : cmds_names_(to_suggestions(cmds)), prompt_(prompt),
        parser_(std::make_unique<cmds::ParserDefault>(std::move(cmds))) {}

public:
  explicit ShellState(cmds_t cmds = {}, std::string_view prompt = "")
      : ShellState(0,
                   cmds::helpize(std::move(cmds), colorize::cgreen,
                                 colorize::cblue, {}),
                   prompt) {}
  ~ShellState() override = default;

  ShellState(ShellState const &) = delete;
  ShellState &operator=(ShellState const &) = delete;

  ShellState(ShellState &&) noexcept = default;
  ShellState &operator=(ShellState &&) noexcept = default;

  [[nodiscard]] suggestions_t suggestions() const override {
    return cmds_names_;
  }
  [[nodiscard]] prompt_t prompt() const override { return prompt_; }

  cmds::IParser &parser() noexcept { return *parser_; }

private:
  suggestions_t cmds_names_;
  prompt_t prompt_;
  std::unique_ptr<cmds::IParser> parser_;
};

} // namespace ublk::cli
