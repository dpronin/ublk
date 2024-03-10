#include "target.hpp"

namespace ublk::raid5 {

uint64_t Target::stripe_id_to_parity_id(uint64_t stripe_id) const noexcept {
  return hs_.size() - (stripe_id % hs_.size()) - 1;
}

} // namespace ublk::raid5
