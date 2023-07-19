#pragma once

#include <cassert>

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include "readline/readline.h"

#include "suggestions_lister_interface.hpp"

#include "types.hpp"

namespace cfq {

class Readline {
  std::shared_ptr<ISuggestionsLister> suggester_;

public:
  static Readline &instance();

  user_input_t operator()(std::string_view prompt = "");

  void set_suggester(std::shared_ptr<ISuggestionsLister> suggester) {
    suggester_ = std::move(suggester);
    assert(suggester_);
  }

  [[nodiscard]] auto suggestions() const { return suggester_->suggestions(); }

private:
  explicit Readline(rl_completion_func_t completer) {
    rl_attempted_completion_function = completer;
  }
};

} // namespace cfq
