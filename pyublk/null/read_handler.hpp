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

  ssize_t handle(std::span<std::byte> buf,
                 __off64_t offset [[maybe_unused]]) noexcept override {
    return buf.size();
  }
};

} // namespace ublk::null
