#include "target.hpp"

namespace ublk::raid4 {

uint64_t Target::stripe_id_to_parity_id(uint64_t stripe_id
                                        [[maybe_unused]]) noexcept {
  return hs_.size() - 1;
}

} // namespace ublk::raid4
