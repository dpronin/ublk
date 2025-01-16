#pragma once

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include <gsl/assert>

#include "flq_submitter_interface.hpp"

namespace ublk {

class FLQSubmitterComposite : public IFLQSubmitter {
public:
  explicit FLQSubmitterComposite(std::vector<std::shared_ptr<IFLQSubmitter>> hs)
      : hs_(std::move(hs)) {
    Ensures(std::ranges::all_of(
        hs_, [](auto const &h) { return static_cast<bool>(h); }));
  }
  ~FLQSubmitterComposite() override = default;

  FLQSubmitterComposite(FLQSubmitterComposite const &) = delete;
  FLQSubmitterComposite &operator=(FLQSubmitterComposite const &) = delete;

  FLQSubmitterComposite(FLQSubmitterComposite &&) = delete;
  FLQSubmitterComposite &operator=(FLQSubmitterComposite &&) = delete;

  int submit(std::shared_ptr<flush_query> fq) noexcept override {
    std::ranges::for_each(
        hs_, [fq = std::move(fq)](auto const &h) { h->submit(fq); });
    return 0;
  }

private:
  std::vector<std::shared_ptr<IFLQSubmitter>> hs_;
};

} // namespace ublk
