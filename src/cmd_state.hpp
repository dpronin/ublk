#pragma once

#include <algorithm>
#include <ranges>
#include <string_view>
#include <utility>

#include "cli_state.hpp"
#include "cmd_help.hpp"
#include "cmd_parser_default.hpp"
#include "types.hpp"

namespace cfq {

class CmdState : public CliState {
private:
  suggestions_t to_suggestions(cmds_t const &cmds) {
    suggestions_t cmds_names{cmds.size()};
    auto named = cmds | std::views::transform(
                            [](auto const &cmd) { return std::get<0>(cmd); });
    std::ranges::copy(named, std::begin(cmds_names));
    return cmds_names;
  }

private:
  explicit CmdState(int, cmds_t cmds = {}, std::string_view prompt = "")
      : cmds_names_(to_suggestions(cmds)), prompt_(prompt),
        parser_(std::make_unique<CmdParserDefault>(std::move(cmds))) {}

public:
  explicit CmdState(cmds_t cmds = {}, std::string_view prompt = "")
      : CmdState(0, cmds_with_help(std::move(cmds)), prompt) {}
  ~CmdState() override = default;

  CmdState(CmdState const &) = delete;
  CmdState &operator=(CmdState const &) = delete;

  CmdState(CmdState &&) noexcept = default;
  CmdState &operator=(CmdState &&) noexcept = default;

  [[nodiscard]] suggestions_t suggestions() const override {
    return cmds_names_;
  }
  [[nodiscard]] prompt_t prompt() const override { return prompt_; }

  ICmdParser &parser() noexcept { return *parser_; }

private:
  suggestions_t cmds_names_;
  prompt_t prompt_;
  std::unique_ptr<ICmdParser> parser_;
};

} // namespace cfq
