#pragma once

#include <cassert>

#include <memory>
#include <utility>

#include "target.hpp"
#include "write_handler_interface.hpp"

namespace ublk::raid5 {

class WriteHandler : public IWriteHandler {
public:
  explicit WriteHandler(std::shared_ptr<Target> target)
      : target_(std::move(target)) {
    assert(target_);
  }
  ~WriteHandler() override = default;

  WriteHandler(WriteHandler const &) = default;
  WriteHandler &operator=(WriteHandler const &) = default;

  WriteHandler(WriteHandler &&) = default;
  WriteHandler &operator=(WriteHandler &&) = default;

  int submit(std::shared_ptr<write_query> wq) noexcept override {
    return target_->process(std::move(wq));
  }

private:
  std::shared_ptr<Target> target_;
};

} // namespace ublk::raid5
