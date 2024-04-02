#pragma once

#include <cassert>

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "flq_submitter_interface.hpp"

namespace ublk {

class FlushHandlerComposite : public IFLQSubmitter {
public:
  explicit FlushHandlerComposite(std::vector<std::shared_ptr<IFLQSubmitter>> hs)
      : hs_(std::move(hs)) {
    assert(std::ranges::all_of(
        hs_, [](auto const &h) { return static_cast<bool>(h); }));
  }
  ~FlushHandlerComposite() override = default;

  FlushHandlerComposite(FlushHandlerComposite const &) = delete;
  FlushHandlerComposite &operator=(FlushHandlerComposite const &) = delete;

  FlushHandlerComposite(FlushHandlerComposite &&) = delete;
  FlushHandlerComposite &operator=(FlushHandlerComposite &&) = delete;

  int submit(std::shared_ptr<flush_query> fq) noexcept override {
    std::ranges::for_each(
        hs_, [fq = std::move(fq)](auto const &h) { h->submit(fq); });
    return 0;
  }

private:
  std::vector<std::shared_ptr<IFLQSubmitter>> hs_;
};

} // namespace ublk
