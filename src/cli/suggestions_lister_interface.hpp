#pragma once

#include "types.hpp"

namespace ublk::cli {

class ISuggestionsLister {
public:
  ISuggestionsLister() = default;
  virtual ~ISuggestionsLister() = default;

  ISuggestionsLister(ISuggestionsLister const &) = default;
  ISuggestionsLister &operator=(ISuggestionsLister const &) = default;

  ISuggestionsLister(ISuggestionsLister &&) noexcept = default;
  ISuggestionsLister &operator=(ISuggestionsLister &&) noexcept = default;

  [[nodiscard]] virtual suggestions_t suggestions() const = 0;
};

} // namespace ublk::cli
