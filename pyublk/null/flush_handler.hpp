#pragma once

#include "flush_handler_interface.hpp"

namespace ublk::null {

class FlushHandler : public IFlushHandler {
public:
  FlushHandler() = default;
  ~FlushHandler() override = default;

  FlushHandler(FlushHandler const &) = default;
  FlushHandler &operator=(FlushHandler const &) = default;

  FlushHandler(FlushHandler &&) = default;
  FlushHandler &operator=(FlushHandler &&) = default;

  int handle() noexcept override { return 0; }
};

} // namespace ublk::null
