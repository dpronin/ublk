#pragma once

#include <cassert>

#include <memory>
#include <utility>

#include "flq_submitter_interface.hpp"
#include "target.hpp"

namespace ublk::def {

class FLQSubmitter : public IFLQSubmitter {
public:
  explicit FLQSubmitter(std::shared_ptr<Target> target)
      : target_(std::move(target)) {
    assert(target_);
  }
  ~FLQSubmitter() override = default;

  FLQSubmitter(FLQSubmitter const &) = delete;
  FLQSubmitter &operator=(FLQSubmitter const &) = delete;

  FLQSubmitter(FLQSubmitter &&) = delete;
  FLQSubmitter &operator=(FLQSubmitter &&) = delete;

  int submit(std::shared_ptr<flush_query> fq) noexcept override {
    return target_->process(std::move(fq));
  }

private:
  std::shared_ptr<Target> target_;
};

} // namespace ublk::def
