#include "target.hpp"

#include <cstdint>

#include <memory>
#include <queue>
#include <string>
#include <utility>
#include <vector>

#include <boost/sml.hpp>

#include <gsl/assert>

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

  std::string state() const {
    auto r{std::string{}};

    fsm_.visit_current_states([&r](auto s) {
      r += s.c_str();
      r.push_back(',');
    });
    r.pop_back();

    return r;
  }

  int process(std::shared_ptr<read_query> rq) noexcept {
    Expects(rq);

    auto *p_rq = rq.get();
    rq = p_rq->subquery(0, p_rq->buf().size(), p_rq->offset(),
                        [this, rq = std::move(rq)](read_query const &new_rq) {
                          if (new_rq.err()) [[unlikely]] {
                            rq->set_err(new_rq.err());
                            fsm_.process_event(fsm::ev::fail{});
                          }
                        });

    fsm::ev::rq e{.rq = std::move(rq), .r = 0};
    fsm_.process_event(e);

    return e.r;
  }

  int process(std::shared_ptr<write_query> wq) noexcept {
    Expects(wq);

    auto *p_wq = wq.get();
    wq = p_wq->subquery(0, p_wq->buf().size(), p_wq->offset(),
                        [this, wq = std::move(wq)](write_query const &new_wq) {
                          if (new_wq.err()) [[unlikely]] {
                            wq->set_err(new_wq.err());
                            fsm_.process_event(fsm::ev::fail{});
                          }
                        });

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
  mutable boost::sml::sm<fsm::transition_table,
                         boost::sml::process_queue<std::queue>>
      fsm_;
};

Target::Target(
    uint64_t strip_sz, std::vector<std::shared_ptr<IRWHandler>> hs,
    std::function<uint64_t(uint64_t stripe_id)> stripe_id_to_parity_id)
    : pimpl_(std::make_unique<impl>(strip_sz, std::move(hs),
                                    std::move(stripe_id_to_parity_id))) {}

Target::~Target() noexcept = default;

Target::Target(Target &&) noexcept = default;
Target &Target::operator=(Target &&) noexcept = default;

std::string Target::state() const { return pimpl_->state(); }

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
