#pragma once

#include <cassert>

#include <memory>
#include <utility>

#include "target.hpp"
#include "wrq_submitter_interface.hpp"

namespace ublk::inmem {

class WRQSubmitter final : public IWRQSubmitter {
public:
  explicit WRQSubmitter(std::shared_ptr<Target> target)
      : target_(std::move(target)) {
    assert(target_);
  }
  ~WRQSubmitter() override = default;

  WRQSubmitter(WRQSubmitter const &) = delete;
  WRQSubmitter &operator=(WRQSubmitter const &) = delete;

  WRQSubmitter(WRQSubmitter &&) = delete;
  WRQSubmitter &operator=(WRQSubmitter &&) = delete;

  int submit(std::shared_ptr<write_query> wq) noexcept override {
    return target_->process(std::move(wq));
  }

private:
  std::shared_ptr<Target> target_;
};

} // namespace ublk::inmem
