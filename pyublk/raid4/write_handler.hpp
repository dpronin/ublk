#pragma once

#include <cassert>

#include <memory>
#include <utility>

#include "target.hpp"
#include "write_handler_interface.hpp"

namespace ublk::raid4 {

class WriteHandler : public IWriteHandler {
public:
  explicit WriteHandler(std::shared_ptr<Target> target)
      : target_(std::move(target)) {
    assert(target_);
  }
  ~WriteHandler() override = default;

  WriteHandler(WriteHandler const &) = delete;
  WriteHandler &operator=(WriteHandler const &) = delete;

  WriteHandler(WriteHandler &&) = delete;
  WriteHandler &operator=(WriteHandler &&) = delete;

  int submit(std::shared_ptr<write_query> wq) noexcept override {
    return target_->process(std::move(wq));
  }

private:
  std::shared_ptr<Target> target_;
};

} // namespace ublk::raid4
