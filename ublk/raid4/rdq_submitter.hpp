#pragma once

#include <cassert>

#include <memory>
#include <utility>

#include "rdq_submitter_interface.hpp"
#include "target.hpp"

namespace ublk::raid4 {

class RDQSubmitter : public IRDQSubmitter {
public:
  explicit RDQSubmitter(std::shared_ptr<Target> target)
      : target_(std::move(target)) {
    assert(target_);
  }
  ~RDQSubmitter() override = default;

  RDQSubmitter(RDQSubmitter const &) = delete;
  RDQSubmitter &operator=(RDQSubmitter const &) = delete;

  RDQSubmitter(RDQSubmitter &&) = delete;
  RDQSubmitter &operator=(RDQSubmitter &&) = delete;

  int submit(std::shared_ptr<read_query> rq) noexcept override {
    return target_->process(std::move(rq));
  }

private:
  std::shared_ptr<Target> target_;
};

} // namespace ublk::raid4
