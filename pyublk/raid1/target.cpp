#include "target.hpp"

#include <cassert>
#include <cerrno>
#include <cstdint>

#include <algorithm>
#include <memory>
#include <queue>
#include <utility>
#include <vector>

#include <boost/sml.hpp>

#include "utils/utility.hpp"

#include "mm/mem.hpp"
#include "mm/mem_types.hpp"

#include "sector.hpp"

using namespace ublk;

namespace {

class r1 final {
public:
  explicit r1(uint64_t read_strip_sz,
              std::vector<std::shared_ptr<IRWHandler>> hs) noexcept;

  int process(std::shared_ptr<read_query> rq) noexcept;
  int process(std::shared_ptr<write_query> wq) noexcept;

private:
  struct static_cfg {
    uint64_t read_strip_sz;
  };
  mm::uptrwd<static_cfg const> static_cfg_;

  uint32_t next_hid_;
  std::vector<std::shared_ptr<IRWHandler>> hs_;
};

r1::r1(uint64_t read_strip_sz,
       std::vector<std::shared_ptr<IRWHandler>> hs) noexcept
    : next_hid_(0), hs_(std::move(hs)) {
  assert(is_multiple_of(read_strip_sz, kSectorSz));
  assert(!(hs_.size() < 2));
  assert(std::ranges::all_of(
      hs_, [](auto const &h) { return static_cast<bool>(h); }));

  auto cfg = mm::make_unique_aligned<static_cfg>(
      hardware_destructive_interference_size);
  cfg->read_strip_sz = read_strip_sz;

  static_cfg_ = mm::const_uptrwd_cast(std::move(cfg));
}

int r1::process(std::shared_ptr<read_query> rq) noexcept {
  for (size_t rb{0}; rb < rq->buf().size();
       next_hid_ = (next_hid_ + 1) % hs_.size()) {
    auto const chunk_sz{
        std::min(static_cfg_->read_strip_sz, rq->buf().size() - rb),
    };
    auto new_rq{rq->subquery(rb, chunk_sz, rq->offset() + rb, rq)};
    if (auto const res{hs_[next_hid_]->submit(std::move(new_rq))})
        [[unlikely]] {
      return res;
    }
    rb += chunk_sz;
  }
  return 0;
}

int r1::process(std::shared_ptr<write_query> wq) noexcept {
  for (auto const &h : hs_) {
    if (auto const res{h->submit(wq)}) [[unlikely]] {
      return res;
    }
  }
  return 0;
}

namespace fsm {

namespace ev {

struct rq {
  std::shared_ptr<read_query> rq;
  mutable int r;
};

struct wq {
  std::shared_ptr<write_query> wq;
  mutable int r;
};

struct fail {};

} // namespace ev

struct transition_table {
  auto operator()() noexcept {
    using namespace boost::sml;
    return make_transition_table(
        // online state
        *"online"_s +
            event<ev::rq> /
                [](ev::rq const &e, r1 &r, back::process<ev::fail> process) {
                  e.r = r.process(e.rq);
                  if (0 != e.r) [[unlikely]] {
                    process(ev::fail{});
                  }
                },
        "online"_s + event<ev::rq> = "offline"_s,
        "online"_s +
            event<ev::wq> /
                [](ev::wq const &e, r1 &r, back::process<ev::fail> process) {
                  e.r = r.process(e.wq);
                  if (0 != e.r) [[unlikely]] {
                    process(ev::fail{});
                  }
                },
        "online"_s + event<ev::wq> = "offline"_s,
        "online"_s + event<ev::fail> = "offline"_s,
        // offline state
        "offline"_s + event<ev::rq> / [](ev::rq const &e) { e.r = EIO; },
        "offline"_s + event<ev::wq> / [](ev::wq const &e) { e.r = EIO; });
  }
};

} // namespace fsm

} // namespace

namespace ublk::raid1 {

class Target::impl final {
public:
  explicit impl(uint64_t strip_sz, std::vector<std::shared_ptr<IRWHandler>> hs)
      : r1_(strip_sz, std::move(hs)), fsm_(r1_) {}

  std::string state() const {
    auto r{std::string{}};

    fsm_.visit_current_states([&r](auto s) {
      r = s.c_str();
      r.push_back(',');
    });
    r.pop_back();

    return r;
  }

  int process(std::shared_ptr<read_query> rq) noexcept {
    assert(rq);

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
    assert(wq);

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

private:
  r1 r1_;
  boost::sml::sm<fsm::transition_table, boost::sml::process_queue<std::queue>>
      fsm_;
};

Target::Target(uint64_t read_strip_sz,
               std::vector<std::shared_ptr<IRWHandler>> hs)
    : pimpl_(std::make_unique<impl>(read_strip_sz, std::move(hs))) {}

int Target::process(std::shared_ptr<read_query> rq) noexcept {
  return pimpl_->process(std::move(rq));
}

int Target::process(std::shared_ptr<write_query> wq) noexcept {
  return pimpl_->process(std::move(wq));
}

Target::~Target() noexcept = default;

Target::Target(Target &&) noexcept = default;
Target &Target::operator=(Target &&) noexcept = default;

} // namespace ublk::raid1
