#pragma once

#include "wrq_submitter_interface.hpp"

namespace ublk::null {

class WRQSubmitter : public IWRQSubmitter {
public:
  WRQSubmitter() = default;
  ~WRQSubmitter() override = default;

  WRQSubmitter(WRQSubmitter const &) = delete;
  WRQSubmitter &operator=(WRQSubmitter const &) = delete;

  WRQSubmitter(WRQSubmitter &&) = delete;
  WRQSubmitter &operator=(WRQSubmitter &&) = delete;

  int submit(std::shared_ptr<write_query> wq
             [[maybe_unused]]) noexcept override {
    return 0;
  }
};

} // namespace ublk::null
