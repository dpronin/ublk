#include "target.hpp"

#include <memory>
#include <utility>

#include "raidsp/target.hpp"

namespace ublk::raid5 {

class Target::impl final : public raidsp::Target {
public:
  using raidsp::Target::Target;

private:
  uint64_t stripe_id_to_parity_id(uint64_t stripe_id) const noexcept override {
    return hs_.size() - (stripe_id % hs_.size()) - 1;
  }
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
