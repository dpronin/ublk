#pragma once

#include "flush_handler_interface.hpp"

namespace ublk::null {

class FlushHandler : public IFlushHandler {
public:
  FlushHandler() = default;
  ~FlushHandler() override = default;

  FlushHandler(FlushHandler const &) = delete;
  FlushHandler &operator=(FlushHandler const &) = delete;

  FlushHandler(FlushHandler &&) = delete;
  FlushHandler &operator=(FlushHandler &&) = delete;

  int submit(std::shared_ptr<flush_query> fq
             [[maybe_unused]]) noexcept override {
    return 0;
  }
};

} // namespace ublk::null
