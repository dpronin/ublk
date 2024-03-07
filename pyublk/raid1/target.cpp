#include "target.hpp"

#include <cassert>

#include <algorithm>
#include <utility>

#include "utils/utility.hpp"

#include "sector.hpp"

namespace ublk::raid1 {

Target::Target(uint64_t read_len_bytes_per_handler,
               std::vector<std::shared_ptr<IRWHandler>> hs) noexcept
    : read_len_bytes_per_handler_(read_len_bytes_per_handler), next_hid_(0),
      hs_(std::move(hs)) {
  assert(is_multiple_of(read_len_bytes_per_handler_, kSectorSz));
  assert(!(hs_.size() < 2));
  assert(std::ranges::all_of(
      hs_, [](auto const &h) { return static_cast<bool>(h); }));
}

int Target::process(std::shared_ptr<read_query> rq) noexcept {
  for (size_t rb{0}; rb < rq->buf().size();
       next_hid_ = (next_hid_ + 1) % hs_.size()) {
    auto const chunk_sz{
        std::min(read_len_bytes_per_handler_, rq->buf().size() - rb),
    };
    auto new_rq = rq->subquery(rb, chunk_sz, rq->offset() + rb,
                               [rq](read_query const &new_rq) {
                                 if (new_rq.err()) [[unlikely]] {
                                   rq->set_err(new_rq.err());
                                   return;
                                 }
                               });
    if (auto const res = hs_[next_hid_]->submit(std::move(new_rq)))
        [[unlikely]] {
      return res;
    }
    rb += chunk_sz;
  }
  return 0;
}

int Target::process(std::shared_ptr<write_query> wq) noexcept {
  for (auto const &h : hs_) {
    if (auto const res = h->submit(wq)) [[unlikely]] {
      return res;
    }
  }
  return 0;
}

} // namespace ublk::raid1
