#pragma once

#include <iostream>

#include <string>
#include <string_view>

#include <termios.h>
#include <unistd.h>

namespace ublk {

inline auto uread(std::string &user_input, std::string_view prompt = {}) {
  std::cout << prompt;
  std::cin.clear();
  return static_cast<bool>(std::getline(std::cin, user_input));
}

inline auto uread_hidden(std::string &user_input,
                         std::string_view prompt = {}) {
  termios ts{};

  // disabling echoing on STDIN with preserving old flags
  tcgetattr(STDIN_FILENO, &ts);
  auto const old_flags = ts.c_lflag;
  ts.c_lflag &= ~ECHO;
  ts.c_lflag |= ECHONL;
  tcsetattr(STDIN_FILENO, TCSANOW, &ts);

  auto deleter = [old_flags](termios *pts) {
    // restoring flags on STDIN
    tcgetattr(STDIN_FILENO, pts);
    pts->c_lflag = old_flags;
    tcsetattr(STDIN_FILENO, TCSANOW, pts);
  };

  std::unique_ptr<termios, decltype(deleter)> guard{&ts, deleter};

  return uread(user_input, prompt);
}

} // namespace ublk
