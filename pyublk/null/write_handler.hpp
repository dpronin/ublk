#pragma once

#include "write_handler_interface.hpp"

namespace ublk::null {

class WriteHandler : public IWriteHandler {
public:
  WriteHandler() = default;
  ~WriteHandler() override = default;

  WriteHandler(WriteHandler const &) = default;
  WriteHandler &operator=(WriteHandler const &) = default;

  WriteHandler(WriteHandler &&) = default;
  WriteHandler &operator=(WriteHandler &&) = default;

  ssize_t handle(std::span<std::byte const> buf,
                 __off64_t offset [[maybe_unused]]) noexcept override {
    return buf.size();
  }
};

} // namespace ublk::null
