#include "target.hpp"

#include <cassert>
#include <cstdint>

#include <algorithm>
#include <concepts>
#include <memory>
#include <utility>

#include <boost/sml.hpp>

#include "mm/mem.hpp"
#include "mm/mem_types.hpp"

#include "utils/utility.hpp"

#include "read_query.hpp"
#include "write_query.hpp"

using namespace ublk;

namespace {

class r0 {
public:
  explicit r0(uint64_t strip_sz, std::vector<std::shared_ptr<IRWHandler>> hs);

  int process(std::shared_ptr<read_query> rq) noexcept {
    return do_op(std::move(rq));
  }

  int process(std::shared_ptr<write_query> wq) noexcept {
    return do_op(std::move(wq));
  }

private:
  template <typename T>
    requires std::same_as<T, write_query> || std::same_as<T, read_query>
  int do_op(std::shared_ptr<T> wq) noexcept;

  struct cfg_t {
    uint64_t strip_sz;
  };

  mm::uptrwd<cfg_t const> cfg_;
  std::vector<std::shared_ptr<IRWHandler>> hs_;
};

r0::r0(uint64_t strip_sz, std::vector<std::shared_ptr<IRWHandler>> hs)
    : hs_(std::move(hs)) {
  assert(is_power_of_2(strip_sz));
  assert(!hs_.empty());
  assert(std::ranges::all_of(
      hs_, [](auto const &h) { return static_cast<bool>(h); }));

  auto cfg =
      mm::make_unique_aligned<cfg_t>(hardware_destructive_interference_size);
  cfg->strip_sz = strip_sz;

  cfg_ = {
      cfg.release(),
      [d = cfg.get_deleter()](cfg_t const *p) { d(const_cast<cfg_t *>(p)); },
  };
}

template <typename T>
  requires std::same_as<T, write_query> || std::same_as<T, read_query>
int r0::do_op(std::shared_ptr<T> query) noexcept {
  assert(query);
  assert(!query->buf().empty());

  auto strip_id{query->offset() / cfg_->strip_sz};
  auto strip_offset{query->offset() % cfg_->strip_sz};

  for (size_t submitted_bytes{0}; submitted_bytes < query->buf().size();
       ++strip_id, strip_offset = 0) {
    auto const strip_id_in_stripe{strip_id % hs_.size()};
    auto const hid{strip_id_in_stripe};
    auto const &h{hs_[hid]};
    auto const stripe_id{strip_id / hs_.size()};
    auto const subquery_offset{strip_offset + stripe_id * cfg_->strip_sz};
    auto const subquery_sz{
        std::min(cfg_->strip_sz - strip_offset,
                 query->buf().size() - submitted_bytes),
    };
    auto subquery{
        query->subquery(submitted_bytes, subquery_sz, subquery_offset, query),
    };
    if (auto const res{h->submit(std::move(subquery))}) [[unlikely]] {
      return res;
    }
    submitted_bytes += subquery_sz;
  }

  return 0;
}

struct ewr {
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
       *"working"_s + event<ewr> / [](ewr const &e, r0& r){ e.r = r.process(std::move(e.rq)); }
      , "working"_s + event<ewq> / [](ewq const &e, r0& r){ e.r = r.process(std::move(e.wq)); }
    );
  }
};
/* clang-format on */

} // namespace

namespace ublk::raid0 {

class Target::impl {
public:
  explicit impl(uint64_t strip_sz, std::vector<std::shared_ptr<IRWHandler>> hs)
      : r0_(strip_sz, std::move(hs)), fsm_(r0_) {}

  int process(std::shared_ptr<read_query> rq) noexcept {
    ewr e{.rq = std::move(rq), .r = 0};
    fsm_.process_event(e);
    return e.r;
  }

  int process(std::shared_ptr<write_query> wq) noexcept {
    ewq e{.wq = std::move(wq), .r = 0};
    fsm_.process_event(e);
    return e.r;
  }

private:
  r0 r0_;
  boost::sml::sm<transition_table> fsm_;
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
