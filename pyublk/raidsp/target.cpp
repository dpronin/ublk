#include "target.hpp"

#include <cstdint>

#include <memory>
#include <utility>
#include <vector>

#include <boost/sml.hpp>

#include "read_query.hpp"
#include "rw_handler_interface.hpp"
#include "write_query.hpp"

#include "acceptor.hpp"
#include "fsm.hpp"

namespace ublk::raidsp {

class Target::impl final {
public:
  explicit impl(
      uint64_t strip_sz, std::vector<std::shared_ptr<IRWHandler>> hs,
      std::function<uint64_t(uint64_t stripe_id)> stripe_id_to_parity_id)
      : acc_(strip_sz, std::move(hs), std::move(stripe_id_to_parity_id)),
        fsm_(acc_) {}

  int process(std::shared_ptr<read_query> rq) noexcept {
    fsm::ev::rq e{.rq = std::move(rq), .r = 0};
    fsm_.process_event(e);
    return e.r;
  }

  int process(std::shared_ptr<write_query> wq) noexcept {
    fsm::ev::wq e{.wq = std::move(wq), .r = 0};
    fsm_.process_event(e);
    return e.r;
  }

  bool is_stripe_parity_coherent(uint64_t stripe_id) const noexcept {
    fsm::ev::stripecohcheck e{.stripe_id = stripe_id, .r = false};
    fsm_.process_event(e);
    return e.r;
  }

private:
  ublk::raidsp::acceptor acc_;
  mutable boost::sml::sm<fsm::transition_table> fsm_;
};

Target::Target(
    uint64_t strip_sz, std::vector<std::shared_ptr<IRWHandler>> hs,
    std::function<uint64_t(uint64_t stripe_id)> stripe_id_to_parity_id)
    : pimpl_(std::make_unique<impl>(strip_sz, std::move(hs),
                                    std::move(stripe_id_to_parity_id))) {}

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

} // namespace ublk::raidsp
