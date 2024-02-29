#pragma once

#include <cassert>

#include <memory>
#include <utility>

#include "rw_handler_interface.hpp"
#include "write_handler_interface.hpp"

namespace ublk {

class WriteHandler : public IWriteHandler {
public:
  explicit WriteHandler(std::shared_ptr<IRWHandler> rwh)
      : rwh_(std::move(rwh)) {
    assert(rwh_);
  }
  ~WriteHandler() override = default;

  WriteHandler(WriteHandler const &) = default;
  WriteHandler &operator=(WriteHandler const &) = default;

  WriteHandler(WriteHandler &&) = default;
  WriteHandler &operator=(WriteHandler &&) = default;

  int submit(std::shared_ptr<write_query> wq) noexcept override {
    return rwh_->submit(std::move(wq));
  }

private:
  std::shared_ptr<IRWHandler> rwh_;
};

} // namespace ublk
