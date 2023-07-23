#pragma once

#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

namespace ublk::cli {

using user_input_t = std::string;
using cmd_t = std::string;
using suggestions_t = std::vector<cmd_t>;
using arg_t = std::string;
using args_t = std::deque<arg_t>;
using desc_cmd_t = std::string;
class ICmd;
using cmd_exec_t = std::tuple<cmd_t, args_t, desc_cmd_t,
                              std::function<std::unique_ptr<ICmd>(args_t)>>;
using cmds_t = std::vector<cmd_exec_t>;
using prompt_t = std::string;

} // namespace ublk::cli
