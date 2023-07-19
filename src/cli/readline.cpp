#include "readline.hpp"

#include <cstdlib>
#include <cstring>

#include <algorithm>
#include <iterator>
#include <ranges>
#include <string_view>

#include "readline/history.h"
#include "readline/readline.h"

#include "types.hpp"

using namespace cfq::cli;

namespace {

char *completion_generator(const char *text, int state) {
  static suggestions_t matches;
  static size_t match_idx = 0;
  if (0 == state) {
    matches.clear();
    match_idx = 0;
    /* clang-format off */
    std::ranges::copy_if(Readline::instance().suggestions(), std::back_inserter(matches), [text](auto const &pattern) {
        return pattern.starts_with(text);
    });
    /* clang-format on */
  }
  return match_idx < matches.size() ? ::strdup(matches[match_idx++].c_str())
                                    : nullptr;
}

char **completer(const char *text, int /*start*/, int /*end*/) {
  rl_attempted_completion_over = 1;
  return rl_completion_matches(text, completion_generator);
}

} // namespace

Readline &Readline::instance() {
  static Readline rl{completer};
  return rl;
}

user_input_t Readline::operator()(std::string_view prompt /* = {}*/) {
  user_input_t user_input;
  auto const deleter = [](char *p) noexcept(noexcept(std::free)) {
    std::free(p);
  };
  if (std::unique_ptr<char, decltype(deleter)> input = {readline(prompt.data()),
                                                        deleter}) {
    user_input = input.get();
    if (!user_input.empty())
      add_history(user_input.c_str());
  }
  return user_input;
}
