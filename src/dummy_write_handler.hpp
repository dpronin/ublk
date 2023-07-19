#pragma once

#include "write_handler_interface.hpp"

namespace cfq {

class DummyWriteHandler : public IWriteHandler {
public:
  DummyWriteHandler() = default;
  ~DummyWriteHandler() override = default;

  DummyWriteHandler(DummyWriteHandler const &) = default;
  DummyWriteHandler &operator=(DummyWriteHandler const &) = default;

  DummyWriteHandler(DummyWriteHandler &&) = default;
  DummyWriteHandler &operator=(DummyWriteHandler &&) = default;

  int handle(std::span<std::byte const> buf,
             __off64_t offset [[maybe_unused]]) noexcept override {
    return buf.size();
  }
};

} // namespace cfq
