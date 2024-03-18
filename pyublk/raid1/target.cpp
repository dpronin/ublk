#include "target.hpp"

#include <cassert>
#include <cerrno>
#include <cstdint>

#include <algorithm>
#include <memory>
#include <utility>

#include <boost/sml.hpp>

#include "utils/utility.hpp"

#include "mm/mem.hpp"
#include "mm/mem_types.hpp"

#include "sector.hpp"

using namespace ublk;

namespace {

class r1 {
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

  static_cfg_ = {
      cfg.release(),
      [d = cfg.get_deleter()](static_cfg const *p) {
        d(const_cast<static_cfg *>(p));
      },
  };
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

struct erq {
  std::shared_ptr<read_query> rq;
  mutable int r;
};

struct ewq {
  std::shared_ptr<write_query> wq;
  mutable int r;
};

/* clang-format off */
  struct transition_table {
    auto operator()() noexcept {
      using namespace boost::sml;
      return make_transition_table(
         // online state
         *"online"_s + event<erq> [ ([](erq const &e, r1& r){ e.r = r.process(std::move(e.rq)); return 0 == e.r; }) ]
        , "online"_s + event<erq> = "offline"_s
        , "online"_s + event<ewq> [ ([](ewq const &e, r1& r){ e.r = r.process(std::move(e.wq)); return 0 == e.r; }) ]
        , "online"_s + event<ewq> = "offline"_s
         // offline state
        , "offline"_s + event<erq> / [](erq const &e) { e.r = EIO; }
        , "offline"_s + event<ewq> / [](ewq const &e) { e.r = EIO; }
      );
    }
  };
/* clang-format on */

} // namespace

namespace ublk::raid1 {

class Target::impl {
public:
  explicit impl(uint64_t strip_sz, std::vector<std::shared_ptr<IRWHandler>> hs)
      : r1_(strip_sz, std::move(hs)), fsm_(r1_) {}

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

private:
  r1 r1_;
  boost::sml::sm<transition_table> fsm_;
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
