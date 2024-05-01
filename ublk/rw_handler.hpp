#pragma once

#include <cassert>

#include <memory>
#include <utility>

#include "rdq_submitter_interface.hpp"
#include "rw_handler_interface.hpp"
#include "wrq_submitter_interface.hpp"

namespace ublk {

class RWHandler final : public IRWHandler {
public:
  explicit RWHandler(std::shared_ptr<IRDQSubmitter> rh,
                     std::shared_ptr<IWRQSubmitter> wh)
      : rh_(std::move(rh)), wh_(std::move(wh)) {
    assert(rh_);
    assert(wh_);
  }
  ~RWHandler() override = default;

  RWHandler(RWHandler const &) = delete;
  RWHandler &operator=(RWHandler const &) = delete;

  RWHandler(RWHandler &&) = delete;
  RWHandler &operator=(RWHandler &&) = delete;

  int submit(std::shared_ptr<read_query> rq) noexcept override {
    return rh_->submit(std::move(rq));
  }

  int submit(std::shared_ptr<write_query> wq) noexcept override {
    return wh_->submit(std::move(wq));
  }

private:
  std::shared_ptr<IRDQSubmitter> rh_;
  std::shared_ptr<IWRQSubmitter> wh_;
};

} // namespace ublk
