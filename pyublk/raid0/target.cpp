#include "target.hpp"

#include <cassert>
#include <cstdint>

#include <algorithm>
#include <utility>

#include "utils/utility.hpp"

#include "read_query.hpp"
#include "write_query.hpp"

namespace ublk::raid0 {

Target::Target(uint64_t strip_sz, std::vector<std::shared_ptr<IRWHandler>> hs)
    : strip_sz_(strip_sz), hs_(std::move(hs)) {
  assert(is_power_of_2(strip_sz_));
  assert(!hs_.empty());
  assert(std::ranges::all_of(
      hs_, [](auto const &h) { return static_cast<bool>(h); }));
}

int Target::read(uint64_t strip_id_from, uint64_t strip_offset_from,
                 std::shared_ptr<read_query> rq) noexcept {
  auto strip_id{strip_id_from + strip_offset_from / strip_sz_};
  auto strip_offset{strip_offset_from % strip_sz_};

  for (size_t rb{0}; rb < rq->buf().size();) {
    auto const strip_id_in_stripe{strip_id % hs_.size()};
    auto const hid{strip_id_in_stripe};
    auto const &h{hs_[hid]};
    auto const stripe_id{strip_id / hs_.size()};
    auto const h_offset{strip_offset + stripe_id * strip_sz_};
    auto const qbuf_sz{
        std::min(strip_sz_ - strip_offset, rq->buf().size() - rb),
    };
    auto new_rq{
        rq->subquery(rb, qbuf_sz, h_offset,
                     [rq](read_query const &new_rq) {
                       if (new_rq.err()) [[unlikely]] {
                         rq->set_err(new_rq.err());
                         return;
                       }
                     }),
    };
    if (auto const res{h->submit(std::move(new_rq))}) [[unlikely]] {
      return res;
    }
    ++strip_id;
    strip_offset = 0;
    rb += qbuf_sz;
  }

  return 0;
}

int Target::process(std::shared_ptr<read_query> rq) noexcept {
  assert(rq);

  auto const strip_id_from = rq->offset() / strip_sz_;
  auto const strip_offset_from = rq->offset() % strip_sz_;
  return read(strip_id_from, strip_offset_from, std::move(rq));
}

int Target::write(uint64_t strip_id_from, uint64_t strip_offset_from,
                  std::shared_ptr<write_query> wq) noexcept {
  auto strip_id{strip_id_from + strip_offset_from / strip_sz_};
  auto strip_offset{strip_offset_from % strip_sz_};

  for (size_t wb{0}; wb < wq->buf().size();) {
    auto const strip_id_in_stripe{strip_id % hs_.size()};
    auto const hid{strip_id_in_stripe};
    auto const &h{hs_[hid]};
    auto const stripe_id{strip_id / hs_.size()};
    auto const h_offset{strip_offset + stripe_id * strip_sz_};
    auto const qbuf_sz{
        std::min(strip_sz_ - strip_offset, wq->buf().size() - wb),
    };
    auto new_wq{
        wq->subquery(wb, qbuf_sz, h_offset,
                     [wq](write_query const &new_wq) {
                       if (new_wq.err()) [[unlikely]] {
                         wq->set_err(new_wq.err());
                         return;
                       }
                     }),
    };
    if (auto const res{h->submit(std::move(new_wq))}) [[unlikely]] {
      return res;
    }
    ++strip_id;
    strip_offset = 0;
    wb += qbuf_sz;
  }

  return 0;
}

int Target::process(std::shared_ptr<write_query> wq) noexcept {
  assert(wq);

  auto const strip_id_from = wq->offset() / strip_sz_;
  auto const strip_offset_from = wq->offset() % strip_sz_;
  return write(strip_id_from, strip_offset_from, std::move(wq));
}

} // namespace ublk::raid0
