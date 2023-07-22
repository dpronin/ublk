#pragma once

#include <algorithm>
#include <iostream>
#include <iterator>
#include <memory>
#include <numeric>
#include <ostream>
#include <ranges>
#include <stdexcept>
#include <utility>

#include "boost/algorithm/string/join.hpp"
#include "boost/format.hpp"

#include "cmd.hpp"
#include "color.hpp"
#include "types.hpp"

namespace cfq::cli {

class CmdHelp final : public ICmd {
  std::ostream &out_;
  cmds_t cmds_;
  boost::format fmt_;

public:
  explicit CmdHelp(std::ostream &out, cmds_t cmds)
      : out_(out), cmds_(std::move(cmds)) {

    std::ranges::sort(cmds_, std::less<>{},
                      [](auto const &cmd) { return std::get<0>(cmd); });
    auto const getsize = [](auto const &v) { return std::size(v); };
    auto const proj_cmd_name = [=](auto const &cmd) {
      return std::get<0>(cmd);
    };
    auto const proj_cmd_args = [=](auto const &cmd) {
      return std::get<1>(cmd);
    };
    auto const getlen_args = [=](auto const &args) {
      return std::transform_reduce(std::begin(args), std::end(args),
                                   std::max(1zu, std::size(args)) - 1,
                                   std::plus<>{}, getsize);
    };
    auto const cmds_lens = cmds_ | std::views::transform(proj_cmd_name) |
                           std::views::transform(getsize);
    auto const cmds_align_size =
        *std::ranges::max_element(cmds_lens) + colorize::cgreen("").size();
    auto const args_lens = cmds_ | std::views::transform(proj_cmd_args) |
                           std::views::transform(getlen_args);
    auto const args_align_size =
        *std::ranges::max_element(args_lens) + colorize::cblue("").size();

    fmt_ = boost::format{"%-" + std::to_string(cmds_align_size) + "s %-" +
                         std::to_string(args_align_size) + "s %s"};
  }

  void exec() override {
    /* clang-format off */
    for (auto const &cmd : cmds_)
      out_ << (fmt_ % (colorize::cgreen(std::get<0>(cmd)))
                    % (colorize::cblue(boost::algorithm::join(std::get<1>(cmd), " ")))
                    % std::get<2>(cmd))
           << std::endl;
    /* clang-format on */
  }
};

inline cmds_t cmds_with_help(cmds_t cmds) {
  cmds.push_back({"help", {}, "Shows this help page", {}});
  std::get<3>(cmds.back()) = [cmds](args_t &&) {
    return std::make_unique<CmdHelp>(std::cout, cmds);
  };
  return cmds;
}

} // namespace cfq::cli
