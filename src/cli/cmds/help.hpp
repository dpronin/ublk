#pragma once

#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>
#include <iterator>
#include <memory>
#include <numeric>
#include <ostream>
#include <ranges>
#include <stdexcept>
#include <string>
#include <utility>

#include "boost/algorithm/string/join.hpp"
#include "boost/format.hpp"

#include "cli/types.hpp"

#include "cmd.hpp"

namespace ublk::cli::cmds {

using colon_transformer = std::function<std::string(std::string)>;

class Help : public ICmd {
public:
  class impl final : public ICmd {
    std::ostream &out_;
    cmds_t cmds_;
    colon_transformer cmd_colon_transformer_;
    colon_transformer args_colon_transformer_;
    colon_transformer desc_colon_transformer_;

    boost::format fmt_;

  public:
    explicit impl(std::ostream &out, cmds_t cmds,
                  colon_transformer cmd_colon_transformer,
                  colon_transformer args_colon_transformer,
                  colon_transformer desc_colon_transformer)
        : out_(out), cmds_(std::move(cmds)),
          cmd_colon_transformer_(std::move(cmd_colon_transformer)),
          args_colon_transformer_(std::move(args_colon_transformer)),
          desc_colon_transformer_(std::move(desc_colon_transformer)) {

      auto reflector = [](auto s) { return s; };

      if (!cmd_colon_transformer_)
        cmd_colon_transformer_ = reflector;

      if (!args_colon_transformer_)
        args_colon_transformer_ = reflector;

      if (!desc_colon_transformer_)
        desc_colon_transformer_ = reflector;

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
      auto const cmds_align_size = *std::ranges::max_element(cmds_lens) +
                                   cmd_colon_transformer_("").size();
      auto const args_lens = cmds_ | std::views::transform(proj_cmd_args) |
                             std::views::transform(getlen_args);
      auto const args_align_size = *std::ranges::max_element(args_lens) +
                                   args_colon_transformer_("").size();

      fmt_ = boost::format{"%-" + std::to_string(cmds_align_size) + "s %-" +
                           std::to_string(args_align_size) + "s %s"};
    }

    void exec() override {
      /* clang-format off */
      for (auto const &cmd : cmds_)
        out_ << (fmt_ % cmd_colon_transformer_(std::get<0>(cmd))
                      % args_colon_transformer_(boost::algorithm::join(std::get<1>(cmd), " "))
                      % desc_colon_transformer_(std::get<2>(cmd)))
             << std::endl;
      /* clang-format on */
    }
  };

public:
  explicit Help(std::shared_ptr<ICmd> cmd) : cmd_(std::move(cmd)) {
    assert(cmd_);
  }

  void exec() override { cmd_->exec(); }

private:
  std::shared_ptr<ICmd> cmd_;
};

inline cmds_t helpize(cmds_t cmds, colon_transformer cmd_colon_transformer,
                      colon_transformer args_colon_transformer,
                      colon_transformer desc_colon_transformer) {
  cmds.push_back({"help", {}, "Shows this help page", {}});
  auto impl = std::make_shared<Help::impl>(
      std::cout, cmds, std::move(cmd_colon_transformer),
      std::move(args_colon_transformer), std::move(desc_colon_transformer));
  std::get<3>(cmds.back()) =
      [impl = std::move(impl)]([[maybe_unused]] args_t &&args) {
        return std::make_unique<Help>(impl);
      };
  return cmds;
}

} // namespace ublk::cli::cmds
