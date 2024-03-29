#pragma once

#include "read_handler_interface.hpp"

namespace ublk::null {

class ReadHandler : public IReadHandler {
public:
  ReadHandler() = default;
  ~ReadHandler() override = default;

  ReadHandler(ReadHandler const &) = delete;
  ReadHandler &operator=(ReadHandler const &) = delete;

  ReadHandler(ReadHandler &&) = delete;
  ReadHandler &operator=(ReadHandler &&) = delete;

  int submit(std::shared_ptr<read_query> rq
             [[maybe_unused]]) noexcept override {
    return 0;
  }
};

} // namespace ublk::null
