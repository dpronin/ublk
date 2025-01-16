#include "backend.hpp"

#include <cstddef>
#include <cstdint>

#include <algorithm>
#include <utility>

#include <gsl/assert>

#include "mm/mem.hpp"

#include "utils/align.hpp"
#include "utils/utility.hpp"

#include "sector.hpp"

namespace ublk::raid1 {

struct backend::static_cfg {
  uint64_t read_strip_sz;
};

backend::backend(uint64_t read_strip_sz,
                 std::vector<std::shared_ptr<IRWHandler>> hs) noexcept
    : next_hid_(0), hs_(std::move(hs)) {
  Ensures(is_multiple_of(read_strip_sz, kSectorSz));
  Ensures(!(hs_.size() < 2));
  Ensures(std::ranges::all_of(
      hs_, [](auto const &h) { return static_cast<bool>(h); }));

  auto cfg = mm::make_unique_aligned<static_cfg>(
      hardware_destructive_interference_size);
  cfg->read_strip_sz = read_strip_sz;

  static_cfg_ = mm::const_uptrwd_cast(std::move(cfg));
}

backend::~backend() noexcept = default;

backend::backend(backend &&) noexcept = default;
backend &backend::operator=(backend &&) noexcept = default;

int backend::process(std::shared_ptr<read_query> rq) noexcept {
  for (auto rb{0uz}; rb < rq->buf().size();
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

int backend::process(std::shared_ptr<write_query> wq) noexcept {
  for (auto const &h : hs_) {
    if (auto const res{h->submit(wq)}) [[unlikely]] {
      return res;
    }
  }
  return 0;
}

} // namespace ublk::raid1
