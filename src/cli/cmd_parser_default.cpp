#include "cmd_parser_default.hpp"

#include <algorithm>
#include <memory>
#include <ostream>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "types.hpp"
#include "utility.hpp"

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
      std::ostringstream oss;
      oss << "unknown command '" << cmd_name;
      if (!args.empty()) {
        std::ranges::copy(args, std::ostream_iterator<arg_t>(oss << ' ', " "));
        oss << '\b';
      }
      oss << '\'';
      throw std::invalid_argument(oss.str());
    }
  }

  if (!cmd)
    cmd = std::make_unique<CmdNull>();

  return cmd;
}
