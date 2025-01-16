#pragma once

#include <memory>
#include <utility>

#include <gsl/assert>

#include "rdq_submitter_interface.hpp"
#include "rw_handler_interface.hpp"

namespace ublk {

class RDQSubmitter final : public IRDQSubmitter {
public:
  explicit RDQSubmitter(std::shared_ptr<IRWHandler> rwh)
      : rwh_(std::move(rwh)) {
    Ensures(rwh_);
  }
  ~RDQSubmitter() override = default;

  RDQSubmitter(RDQSubmitter const &) = delete;
  RDQSubmitter &operator=(RDQSubmitter const &) = delete;

  RDQSubmitter(RDQSubmitter &&) = delete;
  RDQSubmitter &operator=(RDQSubmitter &&) = delete;

  int submit(std::shared_ptr<read_query> rq) noexcept override {
    return rwh_->submit(std::move(rq));
  }

private:
  std::shared_ptr<IRWHandler> rwh_;
};

} // namespace ublk
