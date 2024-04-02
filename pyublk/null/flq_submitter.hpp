#pragma once

#include "flq_submitter_interface.hpp"

namespace ublk::null {

class FLQSubmitter : public IFLQSubmitter {
public:
  FLQSubmitter() = default;
  ~FLQSubmitter() override = default;

  FLQSubmitter(FLQSubmitter const &) = delete;
  FLQSubmitter &operator=(FLQSubmitter const &) = delete;

  FLQSubmitter(FLQSubmitter &&) = delete;
  FLQSubmitter &operator=(FLQSubmitter &&) = delete;

  int submit(std::shared_ptr<flush_query> fq
             [[maybe_unused]]) noexcept override {
    return 0;
  }
};

} // namespace ublk::null
