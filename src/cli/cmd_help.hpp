#pragma once

#include <iostream>
#include <memory>
#include <ostream>
#include <ranges>
#include <stdexcept>
#include <utility>

#include "boost/algorithm/string/join.hpp"
#include "boost/format.hpp"
#include "boost/range/numeric.hpp"

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
    /* clang-format off */
    auto getsize = [](auto const &cmd)
    {
        return std::size(std::get<0>(cmd))
            + boost::accumulate(std::get<1>(cmd), 0, [](auto sum, auto const &cmd) { return sum + cmd.size(); })
            + std::size(std::get<1>(cmd)) - 1;
    };
    /* clang-format on */
    auto const cmds_lens = cmds_ | std::views::transform(getsize);
    size_t const align_size = *std::ranges::max_element(cmds_lens) +
                              colorize::cgreen("").size() +
                              colorize::cblue("").size();
    fmt_ = boost::format{"%-" + std::to_string(align_size + 1) + "s %s"};
  }

  void exec() override {
    for (auto const &cmd : cmds_)
      out_ << (fmt_ %
               (colorize::cgreen(std::get<0>(cmd)) + ' ' +
                (std::empty(std::get<1>(cmd))
                     ? std::string{}
                     : colorize::cblue(
                           boost::algorithm::join(std::get<1>(cmd), " ")))) %
               std::get<2>(cmd))
           << std::endl;
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
