#pragma once

#include <cassert>

#include <memory>
#include <utility>

#include "read_handler_interface.hpp"
#include "rw_handler_interface.hpp"

namespace ublk {

class ReadHandler : public IReadHandler {
public:
  explicit ReadHandler(std::shared_ptr<IRWHandler> rwh) : rwh_(std::move(rwh)) {
    assert(rwh_);
  }
  ~ReadHandler() override = default;

  ReadHandler(ReadHandler const &) = default;
  ReadHandler &operator=(ReadHandler const &) = default;

  ReadHandler(ReadHandler &&) = default;
  ReadHandler &operator=(ReadHandler &&) = default;

  int submit(std::shared_ptr<read_query> rq) noexcept override {
    return rwh_->submit(std::move(rq));
  }

private:
  std::shared_ptr<IRWHandler> rwh_;
};

} // namespace ublk
