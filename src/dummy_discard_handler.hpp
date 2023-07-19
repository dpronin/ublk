#pragma once

#include "discard_handler_interface.hpp"

namespace cfq {

class DummyDiscardHandler : public IDiscardHandler {
public:
  DummyDiscardHandler() = default;
  ~DummyDiscardHandler() override = default;

  DummyDiscardHandler(DummyDiscardHandler const &) = default;
  DummyDiscardHandler &operator=(DummyDiscardHandler const &) = default;

  DummyDiscardHandler(DummyDiscardHandler &&) = default;
  DummyDiscardHandler &operator=(DummyDiscardHandler &&) = default;

  ssize_t handle(__off64_t offset, size_t size) noexcept override {
    return size;
  }
};

} // namespace cfq
