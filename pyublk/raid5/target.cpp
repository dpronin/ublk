#include "target.hpp"

#include <cstdint>

#include <memory>
#include <utility>

#include "raidsp/target.hpp"

namespace ublk::raid5 {

class Target::impl final {
public:
  explicit impl(uint64_t strip_sz, std::vector<std::shared_ptr<IRWHandler>> hs)
      : target_(strip_sz, hs, [strips_nr = hs.size()](uint64_t stripe_id) {
          return strips_nr - (stripe_id % strips_nr) - 1;
        }) {}

  int process(std::shared_ptr<read_query> rq) noexcept {
    return target_.process(std::move(rq));
  }

  int process(std::shared_ptr<write_query> wq) noexcept {
    return target_.process(std::move(wq));
  }

  bool is_stripe_parity_coherent(uint64_t stripe_id) const noexcept {
    return target_.is_stripe_parity_coherent(stripe_id);
  }

private:
  raidsp::Target target_;
};

Target::Target(uint64_t strip_sz, std::vector<std::shared_ptr<IRWHandler>> hs)
    : pimpl_(std::make_unique<impl>(strip_sz, std::move(hs))) {}

Target::~Target() noexcept = default;

Target::Target(Target &&) noexcept = default;
Target &Target::operator=(Target &&) noexcept = default;

bool Target::is_stripe_parity_coherent(uint64_t stripe_id) const noexcept {
  return pimpl_->is_stripe_parity_coherent(stripe_id);
}

int Target::process(std::shared_ptr<read_query> rq) noexcept {
  return pimpl_->process(std::move(rq));
}
int Target::process(std::shared_ptr<write_query> wq) noexcept {
  return pimpl_->process(std::move(wq));
}

} // namespace ublk::raid5
