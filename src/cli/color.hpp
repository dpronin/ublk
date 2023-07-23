#pragma once

#include <string>
#include <string_view>

namespace ublk::cli {

enum class color_e {
  kRed = 31,
  kGreen = 32,
  kBrown = 33,
  kBlue = 34,
  kPurple = 35,
  kLblue = 36,
  kGrey = 90,
};

inline std::string_view color_escape_reset() noexcept { return "\e[0m"; }

inline std::string color_escape(color_e color) {
  return "\e[" + std::to_string(static_cast<int>(color)) + 'm';
}

inline std::string colored(std::string_view input, color_e color) {
  return color_escape(color) + input.data() + color_escape_reset().data();
}

inline namespace colorize {

inline auto cred(std::string_view input) {
  return colored(input, color_e::kRed);
}

inline auto cgreen(std::string_view input) {
  return colored(input, color_e::kGreen);
}

inline auto cbrown(std::string_view input) {
  return colored(input, color_e::kBrown);
}

inline auto cblue(std::string_view input) {
  return colored(input, color_e::kBlue);
}

inline auto cpurple(std::string_view input) {
  return colored(input, color_e::kPurple);
}

inline auto clblue(std::string_view input) {
  return colored(input, color_e::kLblue);
}

inline auto cgrey(std::string_view input) {
  return colored(input, color_e::kGrey);
}

} // namespace colorize

} // namespace ublk::cli
