#pragma once

#include "types.hpp"

#include <algorithm>
#include <iterator>
#include <ostream>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <string_view>

namespace ublk::cli {

class cli_error : public std::runtime_error {
  static auto make_cli_error_what(std::string_view cmd, args_t const &args,
                                  std::string_view what) {
    std::ostringstream oss;
    oss << "cli error: " << what << ": '" << cmd;
    if (!args.empty()) {
      std::ranges::copy(args, std::ostream_iterator<arg_t>(oss << ' ', " "));
      oss << '\b';
    }
    oss << '\'';
    return oss.str();
  }

public:
  explicit cli_error(std::string_view cmd, args_t const &args,
                     std::string_view what)
      : std::runtime_error(make_cli_error_what(cmd, args, what)) {}
};

} // namespace ublk::cli
