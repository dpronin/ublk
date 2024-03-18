#include "target.hpp"

#include <memory>
#include <utility>

#include <boost/sml.hpp>

#include "raidsp/target.hpp"

using namespace ublk;

namespace {

class r4 final : public raidsp::Target {
public:
  using raidsp::Target::Target;

private:
  uint64_t stripe_id_to_parity_id(uint64_t stripe_id
                                  [[maybe_unused]]) const noexcept override {
    return hs_.size() - 1;
  }
};

struct erq {
  std::shared_ptr<read_query> rq;
  mutable int r;
};

struct ewq {
  std::shared_ptr<write_query> wq;
  mutable int r;
};

struct estripecohcheck {
  uint64_t stripe_id;
  mutable bool r;
};

/* clang-format off */
struct transition_table {
  auto operator()() noexcept {
    using namespace boost::sml;
    return make_transition_table(
       // online state
       *"online"_s + event<erq> [ ([](erq const &e, r4& r){ e.r = r.process(std::move(e.rq)); return 0 == e.r; }) ]
      , "online"_s + event<erq> = "offline"_s
      , "online"_s + event<ewq> [ ([](ewq const &e, r4& r){ e.r = r.process(std::move(e.wq)); return 0 == e.r; }) ]
      , "online"_s + event<ewq> = "offline"_s
      , "online"_s + event<estripecohcheck> / [](estripecohcheck const &e, r4 const& r) { e.r = r.is_stripe_parity_coherent(e.stripe_id); }
       // offline state
      , "offline"_s + event<erq> / [](erq const &e) { e.r = EIO; }
      , "offline"_s + event<ewq> / [](ewq const &e) { e.r = EIO; }
      , "offline"_s + event<estripecohcheck> / [](estripecohcheck const &e) { e.r = false; }
    );
  }
};
/* clang-format on */

} // namespace

namespace ublk::raid4 {

class Target::impl final {
public:
  explicit impl(uint64_t strip_sz, std::vector<std::shared_ptr<IRWHandler>> hs)
      : r4_(strip_sz, std::move(hs)), fsm_(r4_) {}

  int process(std::shared_ptr<read_query> rq) noexcept {
    erq e{.rq = std::move(rq), .r = 0};
    fsm_.process_event(e);
    return e.r;
  }

  int process(std::shared_ptr<write_query> wq) noexcept {
    ewq e{.wq = std::move(wq), .r = 0};
    fsm_.process_event(e);
    return e.r;
  }

  bool is_stripe_parity_coherent(uint64_t stripe_id) const noexcept {
    estripecohcheck e{.stripe_id = stripe_id, .r = false};
    fsm_.process_event(e);
    return e.r;
  }

private:
  r4 r4_;
  mutable boost::sml::sm<transition_table> fsm_;
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

} // namespace ublk::raid4
