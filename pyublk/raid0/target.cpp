#include "target.hpp"

#include <cassert>
#include <cstdint>

#include <algorithm>
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

int Target::read(uint64_t strip_id_from, uint64_t strip_offset_from,
                 std::shared_ptr<read_query> rq) noexcept {
  auto strip_id{strip_id_from + strip_offset_from / cfg_->strip_sz};
  auto strip_offset{strip_offset_from % cfg_->strip_sz};

  for (size_t rb{0}; rb < rq->buf().size();) {
    auto const strip_id_in_stripe{strip_id % hs_.size()};
    auto const hid{strip_id_in_stripe};
    auto const &h{hs_[hid]};
    auto const stripe_id{strip_id / hs_.size()};
    auto const h_offset{strip_offset + stripe_id * cfg_->strip_sz};
    auto const qbuf_sz{
        std::min(cfg_->strip_sz - strip_offset, rq->buf().size() - rb),
    };
    auto new_rq{rq->subquery(rb, qbuf_sz, h_offset, rq)};
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

  auto const strip_id_from = rq->offset() / cfg_->strip_sz;
  auto const strip_offset_from = rq->offset() % cfg_->strip_sz;
  return read(strip_id_from, strip_offset_from, std::move(rq));
}

int Target::write(uint64_t strip_id_from, uint64_t strip_offset_from,
                  std::shared_ptr<write_query> wq) noexcept {
  auto strip_id{strip_id_from + strip_offset_from / cfg_->strip_sz};
  auto strip_offset{strip_offset_from % cfg_->strip_sz};

  for (size_t wb{0}; wb < wq->buf().size();) {
    auto const strip_id_in_stripe{strip_id % hs_.size()};
    auto const hid{strip_id_in_stripe};
    auto const &h{hs_[hid]};
    auto const stripe_id{strip_id / hs_.size()};
    auto const h_offset{strip_offset + stripe_id * cfg_->strip_sz};
    auto const qbuf_sz{
        std::min(cfg_->strip_sz - strip_offset, wq->buf().size() - wb),
    };
    auto new_wq{wq->subquery(wb, qbuf_sz, h_offset, wq)};
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

  auto const strip_id_from = wq->offset() / cfg_->strip_sz;
  auto const strip_offset_from = wq->offset() % cfg_->strip_sz;
  return write(strip_id_from, strip_offset_from, std::move(wq));
}

} // namespace ublk::raid0
