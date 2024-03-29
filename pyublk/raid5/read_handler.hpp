#pragma once

#include <cassert>

#include <memory>
#include <utility>

#include "read_handler_interface.hpp"
#include "target.hpp"

namespace ublk::raid5 {

class ReadHandler : public IReadHandler {
public:
  explicit ReadHandler(std::shared_ptr<Target> target)
      : target_(std::move(target)) {
    assert(target_);
  }
  ~ReadHandler() override = default;

  ReadHandler(ReadHandler const &) = delete;
  ReadHandler &operator=(ReadHandler const &) = delete;

  ReadHandler(ReadHandler &&) = delete;
  ReadHandler &operator=(ReadHandler &&) = delete;

  int submit(std::shared_ptr<read_query> rq) noexcept override {
    return target_->process(std::move(rq));
  }

private:
  std::shared_ptr<Target> target_;
};

} // namespace ublk::raid5
