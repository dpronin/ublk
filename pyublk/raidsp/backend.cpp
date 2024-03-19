#include "backend.hpp"

#include <cassert>

#include <algorithm>
#include <utility>

#include "mm/mem.hpp"

namespace ublk::raidsp {

backend::backend(
    uint64_t strip_sz, std::vector<std::shared_ptr<IRWHandler>> hs,
    std::function<uint64_t(uint64_t stripe_id)> const &stripe_id_to_parity_id)
    : hs_(std::move(hs)), stripe_id_to_parity_id_(stripe_id_to_parity_id) {
  assert(is_power_of_2(strip_sz));
  assert(is_multiple_of(strip_sz, kAlignmentRequiredMin));
  assert(!(hs_.size() < 3));
  assert(std::ranges::all_of(
      hs_, [](auto const &h) { return static_cast<bool>(h); }));
  assert(stripe_id_to_parity_id_);

  auto cfg =
      mm::make_unique_aligned<cfg_t>(hardware_destructive_interference_size);
  cfg->strip_sz = strip_sz;
  cfg->stripe_data_sz = cfg->strip_sz * (hs_.size() - 1);
  cfg->stripe_sz = cfg->stripe_data_sz + cfg->strip_sz;

  static_cfg_ = {
      cfg.release(),
      [d = cfg.get_deleter()](cfg_t const *p) { d(const_cast<cfg_t *>(p)); },
  };
}

int backend::data_skip_parity_read(uint64_t stripe_id_from,
                                   std::shared_ptr<read_query> rq) noexcept {
  assert(!rq->buf().empty());
  assert(rq->offset() < static_cfg_->stripe_data_sz);

  auto stripe_id{stripe_id_from};
  auto stripe_offset{rq->offset()};

  for (size_t rb{0}; rb < rq->buf().size(); ++stripe_id, stripe_offset = 0) {
    auto chunk_sz{
        std::min(static_cfg_->stripe_data_sz - stripe_offset,
                 rq->buf().size() - rb),
    };

    auto hs{handlers_generate(stripe_id)};

    for (size_t hid{stripe_offset / static_cfg_->strip_sz},
         strip_offset = stripe_offset % static_cfg_->strip_sz;
         chunk_sz > 0 && hid < (hs.size() - 1); ++hid, strip_offset = 0) {
      auto const h_offset{stripe_id * static_cfg_->strip_sz + strip_offset};
      auto const h_sz{std::min(static_cfg_->strip_sz - strip_offset, chunk_sz)};
      auto new_rq{rq->subquery(rb, h_sz, h_offset, rq)};
      if (auto const res{hs[hid]->submit(std::move(new_rq))}) [[unlikely]] {
        return res;
      }
      rb += h_sz;
      chunk_sz -= h_sz;
    }
    assert(0 == chunk_sz);
  }

  return 0;
}

int backend::stripe_data_read(uint64_t stripe_id_from,
                              std::shared_ptr<read_query> rq) noexcept {
  assert(!rq->buf().empty());
  assert(!(rq->offset() + rq->buf().size() > static_cfg_->stripe_data_sz));

  return data_skip_parity_read(stripe_id_from, std::move(rq));
}

int backend::stripe_parity_read(uint64_t stripe_id,
                                std::shared_ptr<read_query> rq) noexcept {
  assert(!rq->buf().empty());
  assert(!(rq->offset() + rq->buf().size() > static_cfg_->strip_sz));

  auto const parity_id{stripe_id_to_parity_id(stripe_id)};
  assert(parity_id < hs_.size());

  return hs_[parity_id]->submit(
      rq->subquery(0, std::min(static_cfg_->strip_sz, rq->buf().size()),
                   stripe_id * static_cfg_->strip_sz, rq));
}

std::vector<std::shared_ptr<IRWHandler>>
backend::handlers_generate(uint64_t stripe_id) {
  std::vector<std::shared_ptr<IRWHandler>> hs;
  hs.reserve(hs_.size());

  auto const parity_id{stripe_id_to_parity_id(stripe_id)};
  assert(parity_id < hs_.size());

  std::copy_n(hs_.begin(), parity_id, std::back_inserter(hs));
  std::rotate_copy(hs_.begin() + parity_id, hs_.begin() + parity_id + 1,
                   hs_.end(), std::back_inserter(hs));

  return hs;
}

int backend::stripe_write(uint64_t stripe_id_at,
                          std::shared_ptr<write_query> wqd,
                          std::shared_ptr<write_query> wqp) noexcept {
  assert(wqd);
  assert(!wqd->buf().empty());
  assert(!(wqd->offset() + wqd->buf().size() > static_cfg_->stripe_data_sz));
  assert(wqp);
  assert(!wqp->buf().empty());
  assert(!(wqp->offset() + wqp->buf().size() > static_cfg_->strip_sz));

  auto hs{handlers_generate(stripe_id_at)};

  size_t wb{0};
  for (size_t hid{wqd->offset() / static_cfg_->strip_sz},
       strip_offset = wqd->offset() % static_cfg_->strip_sz;
       wb < wqd->buf().size() && hid < (hs.size() - 1);
       ++hid, strip_offset = 0) {
    auto const sq_off{stripe_id_at * static_cfg_->strip_sz + strip_offset};
    auto const sq_sz{
        std::min(static_cfg_->strip_sz - strip_offset, wqd->buf().size() - wb),
    };
    auto new_wqd{wqd->subquery(wb, sq_sz, sq_off, wqd)};
    if (auto const res{hs[hid]->submit(std::move(new_wqd))}) [[unlikely]] {
      wqd->set_err(res);
      return res;
    }
    wb += sq_sz;
  }
  assert(!(wb < wqd->buf().size()));

  auto new_wqp{
      wqp->subquery(0, wqp->buf().size(),
                    stripe_id_at * static_cfg_->strip_sz + wqp->offset(), wqp),
  };

  if (auto const res{hs.back()->submit(std::move(new_wqp))}) [[unlikely]] {
    wqp->set_err(res);
    return res;
  }

  return 0;
}

} // namespace ublk::raidsp
