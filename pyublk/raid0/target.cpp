#include "target.hpp"

#include <cstdint>

#include <memory>
#include <utility>

#include <boost/sml.hpp>

#include "read_query.hpp"
#include "write_query.hpp"

#include "backend.hpp"
#include "fsm.hpp"

using namespace ublk;

namespace ublk::raid0 {

class Target::impl final {
public:
  explicit impl(uint64_t strip_sz, std::vector<std::shared_ptr<IRWHandler>> hs)
      : be_(std::make_unique<backend>(strip_sz, std::move(hs))), fsm_(*be_) {}

  int process(std::shared_ptr<read_query> rq) noexcept {
    auto *p_rq = rq.get();
    rq = p_rq->subquery(0, p_rq->buf().size(), p_rq->offset(),
                        [this, rq = std::move(rq)](read_query const &new_rq) {
                          if (new_rq.err()) [[unlikely]] {
                            rq->set_err(new_rq.err());
                            fsm_.process_event(fsm::ev::fail{});
                          }
                        });
    fsm::ev::rq e{.rq = rq, .r = 0};
    fsm_.process_event(e);
    return e.r;
  }

  int process(std::shared_ptr<write_query> wq) noexcept {
    auto *p_wq = wq.get();
    wq = p_wq->subquery(0, p_wq->buf().size(), p_wq->offset(),
                        [this, wq = std::move(wq)](write_query const &new_wq) {
                          if (new_wq.err()) [[unlikely]] {
                            wq->set_err(new_wq.err());
                            fsm_.process_event(fsm::ev::fail{});
                          }
                        });
    fsm::ev::wq e{.wq = wq, .r = 0};
    fsm_.process_event(e);
    return e.r;
  }

private:
  std::unique_ptr<backend> be_;
  boost::sml::sm<fsm::transition_table> fsm_;
};

Target::Target(uint64_t strip_sz, std::vector<std::shared_ptr<IRWHandler>> hs)
    : pimpl_(std::make_unique<impl>(strip_sz, std::move(hs))) {}

Target::~Target() noexcept = default;

Target::Target(Target &&) noexcept = default;
Target &Target::operator=(Target &&) noexcept = default;

int Target::process(std::shared_ptr<read_query> rq) noexcept {
  return pimpl_->process(std::move(rq));
}

int Target::process(std::shared_ptr<write_query> wq) noexcept {
  return pimpl_->process(std::move(wq));
}

} // namespace ublk::raid0
