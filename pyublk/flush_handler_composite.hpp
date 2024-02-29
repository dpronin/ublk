#pragma once

#include <cassert>

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "flush_handler_interface.hpp"

namespace ublk {

class FlushHandlerComposite : public IFlushHandler {
public:
  explicit FlushHandlerComposite(std::vector<std::shared_ptr<IFlushHandler>> hs)
      : hs_(std::move(hs)) {
    assert(std::ranges::all_of(
        hs_, [](auto const &h) { return static_cast<bool>(h); }));
  }
  ~FlushHandlerComposite() override = default;

  FlushHandlerComposite(FlushHandlerComposite const &) = default;
  FlushHandlerComposite &operator=(FlushHandlerComposite const &) = default;

  FlushHandlerComposite(FlushHandlerComposite &&) = default;
  FlushHandlerComposite &operator=(FlushHandlerComposite &&) = default;

  int submit(std::shared_ptr<flush_query> fq) noexcept override {
    std::ranges::for_each(
        hs_, [fq = std::move(fq)](auto const &h) { h->submit(fq); });
    return 0;
  }

private:
  std::vector<std::shared_ptr<IFlushHandler>> hs_;
};

} // namespace ublk
