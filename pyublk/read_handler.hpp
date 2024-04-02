#pragma once

#include <cassert>

#include <memory>
#include <utility>

#include "rdq_submitter_interface.hpp"
#include "rw_handler_interface.hpp"

namespace ublk {

class ReadHandler final : public IRDQSubmitter {
public:
  explicit ReadHandler(std::shared_ptr<IRWHandler> rwh) : rwh_(std::move(rwh)) {
    assert(rwh_);
  }
  ~ReadHandler() override = default;

  ReadHandler(ReadHandler const &) = delete;
  ReadHandler &operator=(ReadHandler const &) = delete;

  ReadHandler(ReadHandler &&) = delete;
  ReadHandler &operator=(ReadHandler &&) = delete;

  int submit(std::shared_ptr<read_query> rq) noexcept override {
    return rwh_->submit(std::move(rq));
  }

private:
  std::shared_ptr<IRWHandler> rwh_;
};

} // namespace ublk
