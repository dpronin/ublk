#include "backend.hpp"

#include <cassert>

#include <algorithm>
#include <concepts>
#include <utility>

#include "mm/mem.hpp"
#include "mm/mem_types.hpp"

#include "utils/utility.hpp"

namespace ublk::raid0 {

struct backend::static_cfg {
  uint64_t strip_sz;
};

backend::backend(uint64_t strip_sz, std::vector<std::shared_ptr<IRWHandler>> hs)
    : hs_(std::move(hs)) {
  assert(is_power_of_2(strip_sz));
  assert(!hs_.empty());
  assert(std::ranges::all_of(
      hs_, [](auto const &h) { return static_cast<bool>(h); }));

  auto cfg = mm::make_unique_aligned<static_cfg>(
      hardware_destructive_interference_size);
  cfg->strip_sz = strip_sz;

  static_cfg_ = mm::const_uptrwd_cast(std::move(cfg));
}

backend::~backend() noexcept = default;

backend::backend(backend &&) noexcept = default;
backend &backend::operator=(backend &&) noexcept = default;

template <typename T>
  requires std::same_as<T, write_query> || std::same_as<T, read_query>
int backend::do_op(std::shared_ptr<T> query) noexcept {
  assert(query);
  assert(!query->buf().empty());

  auto strip_id{query->offset() / static_cfg_->strip_sz};
  auto strip_offset{query->offset() % static_cfg_->strip_sz};

  for (size_t submitted_bytes{0}; submitted_bytes < query->buf().size();
       ++strip_id, strip_offset = 0) {
    auto const strip_id_in_stripe{strip_id % hs_.size()};
    auto const hid{strip_id_in_stripe};
    auto const &h{hs_[hid]};
    auto const stripe_id{strip_id / hs_.size()};
    auto const subquery_offset{strip_offset +
                               stripe_id * static_cfg_->strip_sz};
    auto const subquery_sz{
        std::min(static_cfg_->strip_sz - strip_offset,
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

int backend::process(std::shared_ptr<read_query> rq) noexcept {
  return do_op(std::move(rq));
}

int backend::process(std::shared_ptr<write_query> wq) noexcept {
  return do_op(std::move(wq));
}

} // namespace ublk::raid0
