#pragma once

#include "raidsp/target.hpp"

namespace ublk::raid5 {

class Target : public raidsp::Target {

public:
  using raidsp::Target::Target;

  ~Target() override = default;

  Target(Target const &) = delete;
  Target &operator=(Target const &) = delete;

  Target(Target &&) = default;
  Target &operator=(Target &&) = default;

  uint64_t stripe_id_to_parity_id(uint64_t stripe_id) const noexcept override;
};

} // namespace ublk::raid5
