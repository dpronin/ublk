#pragma once

#include <cassert>

#include <memory>
#include <utility>

#include "rw_handler_interface.hpp"
#include "wrq_submitter_interface.hpp"

namespace ublk {

class WRQSubmitter final : public IWRQSubmitter {
public:
  explicit WRQSubmitter(std::shared_ptr<IRWHandler> rwh)
      : rwh_(std::move(rwh)) {
    assert(rwh_);
  }
  ~WRQSubmitter() override = default;

  WRQSubmitter(WRQSubmitter const &) = delete;
  WRQSubmitter &operator=(WRQSubmitter const &) = delete;

  WRQSubmitter(WRQSubmitter &&) = delete;
  WRQSubmitter &operator=(WRQSubmitter &&) = delete;

  int submit(std::shared_ptr<write_query> wq) noexcept override {
    return rwh_->submit(std::move(wq));
  }

private:
  std::shared_ptr<IRWHandler> rwh_;
};

} // namespace ublk
