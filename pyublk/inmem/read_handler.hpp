#pragma once

#include <cassert>

#include <memory>
#include <utility>

#include "read_handler_interface.hpp"
#include "target.hpp"

namespace ublk::inmem {

class ReadHandler : public IReadHandler {
public:
  explicit ReadHandler(std::shared_ptr<Target> target)
      : target_(std::move(target)) {
    assert(target_);
  }
  ~ReadHandler() override = default;

  ReadHandler(ReadHandler const &) = default;
  ReadHandler &operator=(ReadHandler const &) = default;

  ReadHandler(ReadHandler &&) = default;
  ReadHandler &operator=(ReadHandler &&) = default;

  int submit(std::shared_ptr<read_query> rq) noexcept override {
    return target_->process(std::move(rq));
  }

private:
  std::shared_ptr<Target> target_;
};

} // namespace ublk::inmem
