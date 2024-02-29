#pragma once

#include <cassert>

#include <memory>
#include <utility>

#include "read_handler_interface.hpp"
#include "rw_handler_interface.hpp"
#include "write_handler_interface.hpp"

namespace ublk {

class RWHandler : public IRWHandler {
public:
  explicit RWHandler(std::shared_ptr<IReadHandler> rh,
                     std::shared_ptr<IWriteHandler> wh)
      : rh_(std::move(rh)), wh_(std::move(wh)) {
    assert(rh_);
    assert(wh_);
  }
  ~RWHandler() override = default;

  RWHandler(RWHandler const &) = default;
  RWHandler &operator=(RWHandler const &) = default;

  RWHandler(RWHandler &&) = default;
  RWHandler &operator=(RWHandler &&) = default;

  int submit(std::shared_ptr<read_query> rq) noexcept override {
    return rh_->submit(std::move(rq));
  }

  int submit(std::shared_ptr<write_query> wq) noexcept override {
    return wh_->submit(std::move(wq));
  }

private:
  std::shared_ptr<IReadHandler> rh_;
  std::shared_ptr<IWriteHandler> wh_;
};

} // namespace ublk
