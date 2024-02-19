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

  ssize_t handle([[maybe_unused]] __off64_t offset,
                 size_t size) noexcept override {
    return size;
  }
};

} // namespace ublk::null
