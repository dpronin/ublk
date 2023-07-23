#pragma once

#include "read_handler_interface.hpp"

namespace ublk {

class DummyReadHandler : public IReadHandler {
public:
  DummyReadHandler() = default;
  ~DummyReadHandler() override = default;

  DummyReadHandler(DummyReadHandler const &) = default;
  DummyReadHandler &operator=(DummyReadHandler const &) = default;

  DummyReadHandler(DummyReadHandler &&) = default;
  DummyReadHandler &operator=(DummyReadHandler &&) = default;

  ssize_t handle(std::span<std::byte> buf,
                 __off64_t offset [[maybe_unused]]) noexcept override {
    return buf.size();
  }
};

} // namespace ublk
