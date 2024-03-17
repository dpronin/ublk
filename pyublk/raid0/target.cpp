#include "target.hpp"

#include <cassert>
#include <cstdint>

#include <algorithm>
#include <concepts>
#include <utility>

#include "mm/mem.hpp"

#include "utils/utility.hpp"

#include "read_query.hpp"
#include "write_query.hpp"

namespace ublk::raid0 {

Target::Target(uint64_t strip_sz, std::vector<std::shared_ptr<IRWHandler>> hs)
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
int Target::do_op(std::shared_ptr<T> query) noexcept {
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

int Target::process(std::shared_ptr<read_query> rq) noexcept {
  return do_op(std::move(rq));
}

int Target::process(std::shared_ptr<write_query> wq) noexcept {
  return do_op(std::move(wq));
}

} // namespace ublk::raid0
