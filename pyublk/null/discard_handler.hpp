#pragma once

#include "discard_handler_interface.hpp"

namespace ublk::null {

class DiscardHandler : public IDiscardHandler {
public:
  DiscardHandler() = default;
  ~DiscardHandler() override = default;

  DiscardHandler(DiscardHandler const &) = default;
  DiscardHandler &operator=(DiscardHandler const &) = default;

  DiscardHandler(DiscardHandler &&) = default;
  DiscardHandler &operator=(DiscardHandler &&) = default;

  int submit(std::shared_ptr<discard_query> dq
             [[maybe_unused]]) noexcept override {
    return 0;
  }
};

} // namespace ublk::null
