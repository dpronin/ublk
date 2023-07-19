#pragma once

#include "suggestions_lister_interface.hpp"

#include "types.hpp"

namespace cfq {

class CliState : public ISuggestionsLister {
public:
  CliState() = default;
  ~CliState() override = default;

  CliState(CliState const &) = default;
  CliState &operator=(CliState const &) = default;

  CliState(CliState &&) noexcept = default;
  CliState &operator=(CliState &&) noexcept = default;

  [[nodiscard]] virtual prompt_t prompt() const = 0;
};

} // namespace cfq
