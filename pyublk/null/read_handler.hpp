#pragma once

#include "read_handler_interface.hpp"

namespace ublk::null {

class ReadHandler : public IReadHandler {
public:
  ReadHandler() = default;
  ~ReadHandler() override = default;

  ReadHandler(ReadHandler const &) = default;
  ReadHandler &operator=(ReadHandler const &) = default;

  ReadHandler(ReadHandler &&) = default;
  ReadHandler &operator=(ReadHandler &&) = default;

  int submit(std::shared_ptr<read_query> rq
             [[maybe_unused]]) noexcept override {
    return 0;
  }
};

} // namespace ublk::null
