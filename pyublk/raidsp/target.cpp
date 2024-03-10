#include "target.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>

#include <algorithm>
#include <iterator>
#include <memory>
#include <span>
#include <utility>

#include "mm/generic_allocators.hpp"

#include "utils/algo.hpp"
#include "utils/math.hpp"
#include "utils/utility.hpp"

#include "parity.hpp"
#include "read_query.hpp"
#include "write_query.hpp"

namespace ublk::raidsp {

Target::Target(uint64_t strip_sz, std::vector<std::shared_ptr<IRWHandler>> hs)
    : strip_sz_(strip_sz), stripe_data_sz_(strip_sz_ * (hs.size() - 1)),
      hs_(std::move(hs)),
      stripe_w_locker_(0, mm::allocator::cache_line_aligned<uint64_t>::value) {
  assert(is_power_of_2(strip_sz_));
  assert(is_multiple_of(strip_sz_, kCachedStripeAlignment));
  assert(!(hs_.size() < 3));
  assert(std::ranges::all_of(
      hs_, [](auto const &h) { return static_cast<bool>(h); }));

  stripe_pool_ = std::make_unique<mm::mem_chunk_pool>(
      kCachedStripeAlignment, stripe_data_sz_ + strip_sz_);
  stripe_parity_pool_ =
      std::make_unique<mm::mem_chunk_pool>(kCachedParityAlignment, strip_sz_);
}

int Target::read_data_skip_parity(uint64_t stripe_id_from,
                                  std::shared_ptr<read_query> rq) noexcept {
  assert(!rq->buf().empty());
  assert(rq->offset() < stripe_data_sz_);

  auto stripe_id{stripe_id_from};
  auto stripe_offset{rq->offset()};

  for (size_t rb{0}; rb < rq->buf().size();) {
    auto chunk_sz{
        std::min(stripe_data_sz_ - stripe_offset, rq->buf().size() - rb),
    };

    auto hs = stripe_id_to_handlers(stripe_id);

    for (size_t hid = stripe_offset / strip_sz_,
                strip_offset = stripe_offset % strip_sz_;
         chunk_sz > 0 && hid < (hs.size() - 1); ++hid, strip_offset = 0) {
      auto const h_offset = stripe_id * strip_sz_ + strip_offset;
      auto const h_sz = std::min(strip_sz_ - strip_offset, chunk_sz);
      auto new_rq =
          rq->subquery(rb, h_sz, h_offset, [rq](read_query const &new_rq) {
            if (new_rq.err()) [[unlikely]] {
              rq->set_err(new_rq.err());
              return;
            }
          });
      if (auto const res{hs[hid]->submit(std::move(new_rq))}) [[unlikely]] {
        return res;
      }
      rb += h_sz;
      chunk_sz -= h_sz;
    }
    assert(0 == chunk_sz);

    stripe_offset = 0;
    ++stripe_id;
  }

  return 0;
}

int Target::read_stripe_data(uint64_t stripe_id_from,
                             std::shared_ptr<read_query> rq) noexcept {
  assert(!rq->buf().empty());
  assert(!(rq->offset() + rq->buf().size() > stripe_data_sz_));

  return read_data_skip_parity(stripe_id_from, std::move(rq));
}

int Target::read_stripe_parity(uint64_t stripe_id,
                               std::shared_ptr<read_query> rq) noexcept {
  assert(!rq->buf().empty());
  assert(!(rq->offset() + rq->buf().size() > strip_sz_));

  auto const parity_id = stripe_id_to_parity_id(stripe_id);
  assert(parity_id < hs_.size());

  return hs_[parity_id]->submit(
      rq->subquery(0, std::min(strip_sz_, rq->buf().size()),
                   stripe_id * strip_sz_, [rq](read_query const &new_rq) {
                     if (new_rq.err()) [[unlikely]] {
                       rq->set_err(new_rq.err());
                       return;
                     }
                   }));
}

std::vector<std::shared_ptr<IRWHandler>>
Target::stripe_id_to_handlers(uint64_t stripe_id) {
  std::vector<std::shared_ptr<IRWHandler>> hs;
  hs.reserve(hs_.size());

  auto const parity_id = stripe_id_to_parity_id(stripe_id);
  assert(parity_id < hs_.size());

  std::copy_n(hs_.begin(), parity_id, std::back_inserter(hs));
  std::rotate_copy(hs_.begin() + parity_id, hs_.begin() + parity_id + 1,
                   hs_.end(), std::back_inserter(hs));

  return hs;
}

int Target::stripe_write(uint64_t stripe_id_at,
                         std::shared_ptr<write_query> wqd,
                         std::shared_ptr<write_query> wqp) noexcept {
  assert(!(wqd->offset() + wqd->buf().size() > stripe_data_sz_));
  assert(!(wqp->offset() + wqp->buf().size() > strip_sz_));

  auto combined_completer_guard = std::shared_ptr<std::nullptr_t>{
      nullptr,
      [=, this](std::nullptr_t *) {
        if (!wqd->err() && !wqp->err()) [[likely]] {
          assert(stripe_id_at < stripe_parity_coherency_state_.size());
          stripe_parity_coherency_state_.set(stripe_id_at);
        }
      },
  };

  auto make_completer = [=](std::shared_ptr<write_query> wq) {
    return [wq, combined_completer_guard](write_query const &new_wq) {
      if (new_wq.err()) [[unlikely]] {
        wq->set_err(new_wq.err());
        return;
      }
    };
  };

  auto hs = stripe_id_to_handlers(stripe_id_at);

  size_t wb{0};
  for (size_t hid = wqd->offset() / strip_sz_,
              strip_offset = wqd->offset() % strip_sz_;
       wb < wqd->buf().size() && hid < (hs.size() - 1);
       ++hid, strip_offset = 0) {
    auto const sq_off = stripe_id_at * strip_sz_ + strip_offset;
    auto const sq_sz =
        std::min(strip_sz_ - strip_offset, wqd->buf().size() - wb);
    auto new_wqd = wqd->subquery(wb, sq_sz, sq_off, make_completer(wqd));
    if (auto const res{hs[hid]->submit(std::move(new_wqd))}) [[unlikely]] {
      wqd->set_err(res);
      return res;
    }
    wb += sq_sz;
  }
  assert(!(wb < wqd->buf().size()));

  auto new_wqp = wqp->subquery(0, wqp->buf().size(),
                               stripe_id_at * strip_sz_ + wqp->offset(),
                               make_completer(wqp));
  if (auto const res{hs.back()->submit(std::move(new_wqp))}) [[unlikely]] {
    wqp->set_err(res);
    return res;
  }

  return 0;
}

int Target::stripe_write(uint64_t stripe_id_at,
                         std::shared_ptr<write_query> wq) noexcept {
  assert(wq);
  assert(!(wq->buf().size() < (stripe_data_sz_ + strip_sz_)));
  auto completer = [wq](write_query const &new_wq) {
    if (new_wq.err()) [[unlikely]] {
      wq->set_err(new_wq.err());
      return;
    }
  };
  auto wqd = wq->subquery(0, stripe_data_sz_, 0, completer);
  auto wqp = wq->subquery(stripe_data_sz_, strip_sz_, 0, completer);
  return stripe_write(stripe_id_at, std::move(wqd), std::move(wqp));
}

int Target::process(std::shared_ptr<read_query> rq) noexcept {
  assert(rq);

  return read_data_skip_parity(rq->offset() / stripe_data_sz_,
                               rq->subquery(0, rq->buf().size(),
                                            rq->offset() % stripe_data_sz_,
                                            [rq](read_query const &new_rq) {
                                              if (new_rq.err()) [[unlikely]] {
                                                rq->set_err(new_rq.err());
                                                return;
                                              }
                                            }));
}

int Target::full_stripe_write_process(
    uint64_t stripe_id_at, std::shared_ptr<write_query> wqd) noexcept {
  auto cached_stripe_parity = stripe_parity_allocate();
  auto cached_stripe_parity_view =
      std::span{cached_stripe_parity.get(), strip_sz_};

  /* Renew Parity of the stripe */
  parity_renew(wqd->buf(), cached_stripe_parity_view);

  auto wqp = write_query::create(
      cached_stripe_parity_view, 0,
      [wqd, cached_stripe_parity = std::shared_ptr{
                std::move(cached_stripe_parity)}](write_query const &new_wqp) {
        if (new_wqp.err()) [[unlikely]] {
          wqd->set_err(new_wqp.err());
          return;
        }
      });

  /*
   * Write Back the chunk including the newly incoming data
   * and the parity computed and updated
   */
  return stripe_write(stripe_id_at, std::move(wqd), std::move(wqp));
}

int Target::process(uint64_t stripe_id,
                    std::shared_ptr<write_query> wq) noexcept {
  assert(!(wq->offset() + wq->buf().size() > stripe_data_sz_));

  /*
   * Read the whole stripe from the backend excluding parity in case we
   * intend to partially modify the stripe
   */
  if (wq->buf().size() < stripe_data_sz_) {
    auto new_cached_stripe = stripe_allocate();
    auto new_cached_stripe_data_view = stripe_data_view(new_cached_stripe);
    auto new_cached_stripe_parity_view = stripe_parity_view(new_cached_stripe);

    if (stripe_parity_coherency_state_[stripe_id]) [[likely]] {
      auto const new_cached_stripe_data_chunk =
          new_cached_stripe_data_view.subspan(wq->offset(), wq->buf().size());
      auto new_rdq = read_query::create(
          new_cached_stripe_data_chunk, wq->offset(),
          [=, this,
           cached_stripe = std::shared_ptr{std::move(new_cached_stripe)}](
              read_query const &rdq) mutable {
            if (rdq.err()) [[unlikely]] {
              wq->set_err(rdq.err());
              return;
            }

            auto new_rpq = read_query::create(
                new_cached_stripe_parity_view, 0,
                [=, this, cached_stripe = std::move(cached_stripe)](
                    read_query const &rpq) mutable {
                  if (rpq.err()) [[unlikely]] {
                    wq->set_err(rpq.err());
                    return;
                  }

                  /*
                   * Renew a required chunk of parity of the stripe by
                   * computing a piece of parity from the scratch basing on
                   * the result of old data being XORed with a new data come
                   * in
                   */
                  math::xor_to(wq->buf(), new_cached_stripe_data_chunk);
                  parity_to(new_cached_stripe_data_chunk,
                            wq->offset() % new_cached_stripe_parity_view.size(),
                            new_cached_stripe_parity_view);

                  auto new_wqp = write_query::create(
                      new_cached_stripe_parity_view, 0,
                      [wq, cached_stripe = std::move(cached_stripe)](
                          write_query const &new_wqp) {
                        if (new_wqp.err()) [[unlikely]] {
                          wq->set_err(new_wqp.err());
                          return;
                        }
                      });

                  /*
                   * Write Back the chunk including the newly incoming data
                   * and the parity computed and updated
                   */
                  if (auto const res = stripe_write(stripe_id, std::move(wq),
                                                    std::move(new_wqp)))
                      [[unlikely]] {
                    return;
                  }
                });
            if (auto const res = read_stripe_parity(
                    stripe_id, std::move(new_rpq))) [[unlikely]] {
              wq->set_err(res);
              return;
            }
          });
      if (auto const res = read_stripe_data(stripe_id, std::move(new_rdq)))
          [[unlikely]] {
        return res;
      }
    } else {
      auto new_rdq = read_query::create(
          new_cached_stripe_data_view, 0,
          [=, this,
           cached_stripe = std::shared_ptr{std::move(new_cached_stripe)}](
              read_query const &rdq) mutable {
            if (rdq.err()) [[unlikely]] {
              wq->set_err(rdq.err());
              return;
            }

            auto const chunk{wq->buf()};

            auto const copy_from = chunk;
            auto const copy_to =
                new_cached_stripe_data_view.subspan(wq->offset(), chunk.size());

            /* Modify the part of the stripe with the new data come in */
            algo::copy(copy_from, copy_to);

            /* Renew Parity of the stripe */
            parity_renew(new_cached_stripe_data_view,
                         new_cached_stripe_parity_view);

            auto new_wqd = write_query::create(
                chunk, wq->offset(), [wq](write_query const &new_wq) {
                  if (new_wq.err()) [[unlikely]] {
                    wq->set_err(new_wq.err());
                    return;
                  }
                });

            auto new_wqp = write_query::create(
                new_cached_stripe_parity_view, 0,
                [wq, cached_stripe_sp = std::shared_ptr{
                         std::move(cached_stripe)}](write_query const &new_wq) {
                  if (new_wq.err()) [[unlikely]] {
                    wq->set_err(new_wq.err());
                    return;
                  }
                });

            /*
             * Write Back the chunk including the newly incoming data
             * and the parity computed and updated
             */
            if (auto const res = stripe_write(stripe_id, std::move(new_wqd),
                                              std::move(new_wqp)))
                [[unlikely]] {
              wq->set_err(res);
              return;
            }
          });
      if (auto const res = read_stripe_data(stripe_id, std::move(new_rdq)))
          [[unlikely]] {
        return res;
      }
    }
    /*
     * Calculate parity based on newly incoming stripe-long chunk and write
     * back the whole stripe including the chunk and parity computed
     */
  } else if (auto const res =
                 full_stripe_write_process(stripe_id, std::move(wq))) {
    return res;
  }

  return 0;
}

int Target::process(std::shared_ptr<write_query> wq) noexcept {
  assert(wq);
  assert(!wq->buf().empty());

  stripe_w_locker_.extend(
      div_round_up(wq->offset() + wq->buf().size(), stripe_data_sz_));
  stripe_parity_coherency_state_.resize(stripe_w_locker_.size());

  auto stripe_id{wq->offset() / stripe_data_sz_};
  auto stripe_offset{wq->offset() % stripe_data_sz_};

  for (size_t wb{0}; wb < wq->buf().size();) {
    auto const chunk_sz{
        std::min(stripe_data_sz_ - stripe_offset, wq->buf().size() - wb),
    };

    auto new_wq = wq->subquery(
        wb, chunk_sz, stripe_offset,
        [wq, stripe_id, this](write_query const &new_wq) {
          if (new_wq.err()) [[unlikely]]
            wq->set_err(new_wq.err());

          for (bool finish = false; !finish;) {
            if (auto next_wq_it = std::ranges::find_if(
                    wqs_pending_,
                    [stripe_id](uint64_t wq_stripe_id) {
                      return stripe_id == wq_stripe_id;
                    },
                    [](auto const &wq_pend) { return wq_pend.first; });
                next_wq_it != wqs_pending_.end()) {
              finish = 0 == process(stripe_id, next_wq_it->second);
              std::iter_swap(next_wq_it, wqs_pending_.end() - 1);
              wqs_pending_.pop_back();
            } else {
              stripe_w_locker_.unlock(stripe_id);
              finish = true;
            }
          }
        });

    if (!stripe_w_locker_.try_lock(stripe_id)) [[unlikely]] {
      wqs_pending_.emplace_back(stripe_id, std::move(new_wq));
    } else if (auto const res{process(stripe_id, std::move(new_wq))})
        [[unlikely]] {
      return res;
    }

    ++stripe_id;
    stripe_offset = 0;
    wb += chunk_sz;
  }

  return 0;
}

} // namespace ublk::raidsp
