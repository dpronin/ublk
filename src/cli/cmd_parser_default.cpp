#include "cmd_parser_default.hpp"

#include <algorithm>
#include <memory>
#include <ranges>
#include <tuple>
#include <utility>

#include "utility.hpp"

#include "cli_error.hpp"
#include "cmd_interface.hpp"
#include "cmd_null.hpp"

using namespace ublk::cli;

CmdParserDefault::CmdParserDefault(cmds_t cmds /* = {}*/)
    : cmds_(std::move(cmds)) {}

std::unique_ptr<ICmd> CmdParserDefault::parse(args_t args) {
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
    cmd = std::make_unique<CmdNull>();

  return cmd;
}
