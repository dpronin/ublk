#pragma once

#include "write_handler_interface.hpp"

namespace ublk::null {

class WriteHandler : public IWriteHandler {
public:
  WriteHandler() = default;
  ~WriteHandler() override = default;

  WriteHandler(WriteHandler const &) = delete;
  WriteHandler &operator=(WriteHandler const &) = delete;

  WriteHandler(WriteHandler &&) = delete;
  WriteHandler &operator=(WriteHandler &&) = delete;

  int submit(std::shared_ptr<write_query> wq
             [[maybe_unused]]) noexcept override {
    return 0;
  }
};

} // namespace ublk::null
