#pragma once

#include "rdq_submitter_interface.hpp"

namespace ublk::null {

class RDQSubmitter : public IRDQSubmitter {
public:
  RDQSubmitter() = default;
  ~RDQSubmitter() override = default;

  RDQSubmitter(RDQSubmitter const &) = delete;
  RDQSubmitter &operator=(RDQSubmitter const &) = delete;

  RDQSubmitter(RDQSubmitter &&) = delete;
  RDQSubmitter &operator=(RDQSubmitter &&) = delete;

  int submit(std::shared_ptr<read_query> rq
             [[maybe_unused]]) noexcept override {
    return 0;
  }
};

} // namespace ublk::null
