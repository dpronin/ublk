#include "target.hpp"

#include <cassert>

#include <algorithm>
#include <utility>

#include "utils/utility.hpp"

#include "mm/mem.hpp"

#include "sector.hpp"

namespace ublk::raid1 {

Target::Target(uint64_t read_len_bytes_per_handler,
               std::vector<std::shared_ptr<IRWHandler>> hs) noexcept
    : next_hid_(0), hs_(std::move(hs)) {
  assert(is_multiple_of(read_len_bytes_per_handler, kSectorSz));
  assert(!(hs_.size() < 2));
  assert(std::ranges::all_of(
      hs_, [](auto const &h) { return static_cast<bool>(h); }));

  auto cfg =
      mm::make_unique_aligned<cfg_t>(hardware_destructive_interference_size);
  cfg->read_len_bytes_per_handler = read_len_bytes_per_handler;

  cfg_ = {
      cfg.release(),
      [d = cfg.get_deleter()](cfg_t const *p) { d(const_cast<cfg_t *>(p)); },
  };
}

int Target::process(std::shared_ptr<read_query> rq) noexcept {
  for (size_t rb{0}; rb < rq->buf().size();
       next_hid_ = (next_hid_ + 1) % hs_.size()) {
    auto const chunk_sz{
        std::min(cfg_->read_len_bytes_per_handler, rq->buf().size() - rb),
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

int Target::process(std::shared_ptr<write_query> wq) noexcept {
  for (auto const &h : hs_) {
    if (auto const res{h->submit(wq)}) [[unlikely]] {
      return res;
    }
  }
  return 0;
}

} // namespace ublk::raid1
