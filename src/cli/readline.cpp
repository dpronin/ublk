#include "readline.hpp"

#include <cstdlib>
#include <cstring>

#include <algorithm>
#include <iostream>
#include <iterator>
#include <ranges>
#include <string_view>

#include "readline/history.h"
#include "readline/readline.h"

#include "types.hpp"

using namespace ublk::cli;

Readline &Readline::instance() {
  static Readline rl{
      +[](const char *text, int /*start*/, int /*end*/) {
        rl_attempted_completion_over = 1;
        return rl_completion_matches(
            text, +[](const char *text, int state) {
              static suggestions_t matches;
              static size_t match_idx = 0;
              if (0 == state) {
                matches.clear();
                match_idx = 0;
                std::ranges::copy_if(Readline::instance().suggestions(),
                                     std::back_inserter(matches),
                                     [text](auto const &pattern) {
                                       return pattern.starts_with(text);
                                     });
              }
              return match_idx < matches.size()
                         ? ::strdup(matches[match_idx++].c_str())
                         : nullptr;
            });
      },
  };
  return rl;
}

user_input_t Readline::operator()(std::string_view prompt /* = {}*/) {
  user_input_t user_input;
  auto const deleter = [](char *p) noexcept(noexcept(std::free)) {
    std::free(p);
  };
  if (auto const input = std::unique_ptr<char, decltype(deleter)>{
          readline(prompt.data()), deleter}) {
    user_input = input.get();
    if (!user_input.empty())
      add_history(user_input.c_str());
  } else {
    std::cout << '\n';
  }
  return user_input;
}
