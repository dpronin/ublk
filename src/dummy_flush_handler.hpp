#pragma once

#include "flush_handler_interface.hpp"

namespace cfq {

class DummyFlushHandler : public IFlushHandler {
public:
  DummyFlushHandler() = default;
  ~DummyFlushHandler() override = default;

  DummyFlushHandler(DummyFlushHandler const &) = default;
  DummyFlushHandler &operator=(DummyFlushHandler const &) = default;

  DummyFlushHandler(DummyFlushHandler &&) = default;
  DummyFlushHandler &operator=(DummyFlushHandler &&) = default;

  int handle() noexcept override { return 0; }
};

} // namespace cfq
