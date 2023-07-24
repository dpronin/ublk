#include "parser_default.hpp"

#include <algorithm>
#include <memory>
#include <ranges>
#include <tuple>
#include <utility>

#include "utility.hpp"

#include "cli/cli_error.hpp"
#include "cli/cmds/cmd_interface.hpp"
#include "cli/cmds/null.hpp"

using namespace ublk::cli;
using namespace ublk::cli::cmds;

ParserDefault::ParserDefault(cmds_t cmds /* = {}*/) : cmds_(std::move(cmds)) {}

std::unique_ptr<ICmd> ParserDefault::parse(args_t args) {
  std::unique_ptr<ICmd> cmd;

  if (args = normalize(std::move(args)); !args.empty()) {
    auto const cmd_name = std::move(args.front());
    args.pop_front();

    if (auto it = std::ranges::find(
            cmds_, cmd_name, [](auto const &cmd) { return std::get<0>(cmd); });
        cmds_.end() != it) {

      cmd = std::get<3>(*it)(std::move(args));
    } else {
      throw cli_error(cmd_name, args, "Unknown Command");
    }
  }

  if (!cmd)
    cmd = std::make_unique<cmds::Null>();

  return cmd;
}
