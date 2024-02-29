#pragma once

#include "raidsp/target.hpp"

namespace ublk::raid4 {

class Target final : public raidsp::Target {
public:
  using raidsp::Target::Target;

protected:
  uint64_t stripe_id_to_parity_id(uint64_t stripe_id) const noexcept override;
};

} // namespace ublk::raid4
