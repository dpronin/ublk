#pragma once

#include <iostream>

#include <string>
#include <string_view>

#include <termios.h>
#include <unistd.h>

#include "types.hpp"

namespace cfq {

inline auto uread(user_input_t &user_input, std::string_view prompt = {}) {
  std::cout << prompt;
  std::cin.clear();
  return static_cast<bool>(std::getline(std::cin, user_input));
}

inline auto uread_hidden(user_input_t &user_input,
                         std::string_view prompt = {}) {
  termios ts{};

  // disabling echoing on STDIN with preserving old flags
  tcgetattr(STDIN_FILENO, &ts);
  auto const old_flags = ts.c_lflag;
  ts.c_lflag &= ~ECHO;
  ts.c_lflag |= ECHONL;
  tcsetattr(STDIN_FILENO, TCSANOW, &ts);

  auto const res = uread(user_input, prompt);

  // restoring flags on STDIN
  tcgetattr(STDIN_FILENO, &ts);
  ts.c_lflag = old_flags;
  tcsetattr(STDIN_FILENO, TCSANOW, &ts);

  return res;
}

} // namespace cfq
