#include "acceptor.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>

#include <algorithm>
#include <memory>
#include <span>
#include <utility>

#include "mm/generic_allocators.hpp"

#include "utils/algo.hpp"
#include "utils/math.hpp"

#include "parity.hpp"

namespace ublk::raidsp {

acceptor::acceptor(
    uint64_t strip_sz, std::vector<std::shared_ptr<IRWHandler>> hs,
    std::function<uint64_t(uint64_t stripe_id)> stripe_id_to_parity_id)
    : be_(std::make_unique<backend>(strip_sz, std::move(hs),
                                    std::move(stripe_id_to_parity_id))),
      stripe_w_locker_(0, mm::allocator::cache_line_aligned<uint64_t>::value),
      stripe_pool_(std::make_unique<mm::mem_chunk_pool>(
          kCachedStripeAlignment, be_->static_cfg().stripe_sz)),
      stripe_parity_pool_(std::make_unique<mm::mem_chunk_pool>(
          kCachedParityAlignment, be_->static_cfg().strip_sz)) {}

bool acceptor::is_stripe_parity_coherent(uint64_t stripe_id) const noexcept {
  return stripe_id < stripe_parity_coherency_state_.size() &&
         stripe_parity_coherency_state_[stripe_id];
}

int acceptor::process(std::shared_ptr<read_query> rq) noexcept {
  assert(rq);

  return be_->data_read(
      rq->offset() / be_->static_cfg().stripe_data_sz,
      rq->subquery(0, rq->buf().size(),
                   rq->offset() % be_->static_cfg().stripe_data_sz, rq));
}

int acceptor::stripe_incoherent_parity_write(
    uint64_t stripe_id_at, std::shared_ptr<write_query> wqd,
    std::shared_ptr<write_query> wqp) noexcept {
  assert(wqd);
  assert(wqp);

  auto combined_completer_guard{
      std::shared_ptr<std::nullptr_t>{
          nullptr,
          [=, this](std::nullptr_t *) {
            assert(stripe_id_at < stripe_parity_coherency_state_.size());
            stripe_parity_coherency_state_.set(stripe_id_at,
                                               !(wqd->err() || wqp->err()));
          },
      },
  };

  auto make_a_new_completer{
      [combined_completer_guard = std::move(combined_completer_guard)](
          std::shared_ptr<write_query> wq) {
        return [wq = std::move(wq),
                combined_completer_guard](write_query const &new_wq) {
          if (new_wq.err()) [[unlikely]] {
            wq->set_err(new_wq.err());
            return;
          }
        };
      },
  };

  auto *p_wqd = wqd.get();
  wqd = write_query::create(p_wqd->buf(), p_wqd->offset(),
                            make_a_new_completer(std::move(wqd)));

  auto *p_wqp = wqp.get();
  wqp = write_query::create(p_wqp->buf(), p_wqp->offset(),
                            make_a_new_completer(std::move(wqp)));

  return be_->stripe_write(stripe_id_at, std::move(wqd), std::move(wqp));
}

int acceptor::stripe_coherent_parity_write(
    uint64_t stripe_id_at, std::shared_ptr<write_query> wqd,
    std::shared_ptr<write_query> wqp) noexcept {
  assert(wqd);
  assert(wqp);

  auto make_a_new_completer{
      [=, this](std::shared_ptr<write_query> wq) {
        return [=, this, wq = std::move(wq)](write_query const &new_wq) {
          if (new_wq.err()) [[unlikely]] {
            wq->set_err(new_wq.err());
            assert(stripe_id_at < stripe_parity_coherency_state_.size());
            stripe_parity_coherency_state_.reset(stripe_id_at);
            return;
          }
        };
      },
  };

  auto *p_wqd = wqd.get();
  wqd = write_query::create(p_wqd->buf(), p_wqd->offset(),
                            make_a_new_completer(std::move(wqd)));

  auto *p_wqp = wqp.get();
  wqp = write_query::create(p_wqp->buf(), p_wqp->offset(),
                            make_a_new_completer(std::move(wqp)));

  return be_->stripe_write(stripe_id_at, std::move(wqd), std::move(wqp));
}

int acceptor::stripe_write(uint64_t stripe_id_at,
                           std::shared_ptr<write_query> wqd,
                           std::shared_ptr<write_query> wqp) noexcept {
  return is_stripe_parity_coherent(stripe_id_at)
             ? stripe_coherent_parity_write(stripe_id_at, std::move(wqd),
                                            std::move(wqp))
             : stripe_incoherent_parity_write(stripe_id_at, std::move(wqd),
                                              std::move(wqp));
}

int acceptor::stripe_data_write(uint64_t stripe_id_at,
                                std::shared_ptr<write_query> wqd) noexcept {
  assert(wqd);
  assert(!wqd->buf().empty());
  assert(0 == wqd->offset());
  assert(wqd->buf().size() == be_->static_cfg().stripe_data_sz);

  auto stripe_parity_buf{stripe_parity_pool_->get()};
  auto stripe_parity_buf_view{
      std::span{stripe_parity_buf.get(), stripe_parity_pool_->chunk_sz()},
  };

  /* Renew Parity of the stripe */
  parity_renew(wqd->buf(), stripe_parity_buf_view);

  auto wqp_completer{
      [wqd, cached_stripe_parity = std::shared_ptr{std::move(
                stripe_parity_buf)}](write_query const &new_wqp) {
        if (new_wqp.err()) [[unlikely]] {
          wqd->set_err(new_wqp.err());
          return;
        }
      },
  };

  auto wqp{
      write_query::create(stripe_parity_buf_view, 0, std::move(wqp_completer)),
  };

  /*
   * Write Back the chunk including the newly incoming full-stripe-sized data
   * and the parity renewed
   */
  return stripe_write(stripe_id_at, std::move(wqd), std::move(wqp));
}

int acceptor::process(uint64_t stripe_id,
                      std::shared_ptr<write_query> wq) noexcept {
  assert(wq);
  assert(!wq->buf().empty());
  assert(!(wq->offset() + wq->buf().size() > be_->static_cfg().stripe_data_sz));

  /*
   * Read the whole stripe from the backend excluding parity in case we
   * intend to partially modify the stripe
   */
  if (wq->buf().size() < be_->static_cfg().stripe_data_sz) {
    auto stripe_buf{stripe_pool_->get()};
    auto const stripe_buf_view{
        std::span<std::byte>{stripe_buf.get(), stripe_pool_->chunk_sz()},
    };
    auto stripe_data_buf_view{
        stripe_buf_view.subspan(0, be_->static_cfg().stripe_data_sz),
    };
    auto const stripe_parity_buf_view{
        stripe_buf_view.subspan(stripe_data_buf_view.size(),
                                stripe_buf_view.size() -
                                    stripe_data_buf_view.size()),
    };

    auto new_rqd{std::shared_ptr<read_query>{}};

    if (stripe_parity_coherency_state_[stripe_id]) [[likely]] {
      stripe_data_buf_view =
          stripe_data_buf_view.subspan(wq->offset(), wq->buf().size());

      auto new_rdq_completer{
          [=, this, stripe_buf = std::shared_ptr{std::move(stripe_buf)}](
              read_query const &rqd) mutable {
            if (rqd.err()) [[unlikely]] {
              wq->set_err(rqd.err());
              return;
            }

            auto new_rpq_completer{
                [=, this, stripe_buf = std::move(stripe_buf)](
                    read_query const &rqp) mutable {
                  if (rqp.err()) [[unlikely]] {
                    wq->set_err(rqp.err());
                    return;
                  }

                  /*
                   * Renew a required chunk of parity of the stripe by
                   * computing a piece of parity from the scratch basing on
                   * the result of old data being XORed with a new data come
                   * in
                   */
                  math::xor_to(wq->buf(), stripe_data_buf_view);
                  parity_to(stripe_data_buf_view, stripe_parity_buf_view,
                            wq->offset() % stripe_parity_buf_view.size());

                  auto wqp{
                      write_query::create(
                          stripe_parity_buf_view, 0,
                          [wq, stripe_buf = std::move(stripe_buf)](
                              write_query const &new_wqp) {
                            if (new_wqp.err()) [[unlikely]] {
                              wq->set_err(new_wqp.err());
                              return;
                            }
                          }),
                  };

                  /*
                   * Write Back the chunk including the newly incoming data
                   * and the parity computed and updated
                   */
                  if (auto const res{
                          stripe_coherent_parity_write(stripe_id, std::move(wq),
                                                       std::move(wqp)),
                      }) [[unlikely]] {
                    return;
                  }
                },
            };

            auto new_rpq{
                read_query::create(stripe_parity_buf_view, 0,
                                   std::move(new_rpq_completer)),
            };

            if (auto const res{
                    be_->parity_read(stripe_id, std::move(new_rpq)),
                }) [[unlikely]] {
              wq->set_err(res);
              return;
            }
          },
      };

      new_rqd = read_query::create(stripe_data_buf_view, wq->offset(),
                                   std::move(new_rdq_completer));
    } else {
      auto new_rdq_completer{
          [=, this, cached_stripe = std::shared_ptr{std::move(stripe_buf)}](
              read_query const &rqd) mutable {
            if (rqd.err()) [[unlikely]] {
              wq->set_err(rqd.err());
              return;
            }

            auto const chunk{wq->buf()};

            auto const copy_from{chunk};
            auto const copy_to{
                stripe_data_buf_view.subspan(wq->offset(), chunk.size()),
            };

            /* Modify the part of the stripe with the new data come in */
            algo::copy(copy_from, copy_to);

            /* Renew Parity of the stripe */
            parity_renew(stripe_data_buf_view, stripe_parity_buf_view);

            auto wqd_completer{
                [wq](write_query const &new_wq) {
                  if (new_wq.err()) [[unlikely]] {
                    wq->set_err(new_wq.err());
                    return;
                  }
                },
            };

            auto new_wqd{
                write_query::create(chunk, wq->offset(),
                                    std::move(wqd_completer)),
            };

            auto wqp_completer{
                [wq, cached_stripe_sp = std::shared_ptr{std::move(
                         cached_stripe)}](write_query const &new_wq) {
                  if (new_wq.err()) [[unlikely]] {
                    wq->set_err(new_wq.err());
                    return;
                  }
                },
            };

            auto new_wqp{
                write_query::create(stripe_parity_buf_view, 0,
                                    std::move(wqp_completer)),
            };

            /*
             * Write Back the chunk including the newly incoming data
             * and the parity computed and updated
             */
            if (auto const res{
                    stripe_incoherent_parity_write(
                        stripe_id, std::move(new_wqd), std::move(new_wqp)),
                }) [[unlikely]] {
              wq->set_err(res);
              return;
            }
          },
      };

      new_rqd = read_query::create(stripe_data_buf_view, 0,
                                   std::move(new_rdq_completer));
    }

    if (auto const res{be_->data_read(stripe_id, std::move(new_rqd))})
        [[unlikely]] {
      return res;
    }

    /*
     * Calculate parity based on newly incoming stripe-long chunk and write
     * back the whole stripe including the chunk and parity computed
     */
  } else if (auto const res{stripe_data_write(stripe_id, std::move(wq))})
      [[unlikely]] {
    return res;
  }

  return 0;
}

int acceptor::process(std::shared_ptr<write_query> wq) noexcept {
  assert(wq);
  assert(!wq->buf().empty());

  stripe_w_locker_.extend(div_round_up(wq->offset() + wq->buf().size(),
                                       be_->static_cfg().stripe_data_sz));
  stripe_parity_coherency_state_.resize(stripe_w_locker_.size());

  auto stripe_id{wq->offset() / be_->static_cfg().stripe_data_sz};
  auto stripe_offset{wq->offset() % be_->static_cfg().stripe_data_sz};

  for (size_t wb{0}; wb < wq->buf().size(); ++stripe_id, stripe_offset = 0) {
    auto const chunk_sz{
        std::min(be_->static_cfg().stripe_data_sz - stripe_offset,
                 wq->buf().size() - wb),
    };

    auto new_wq_completer{
        [wq, stripe_id, this](write_query const &new_wq) {
          if (new_wq.err()) [[unlikely]]
            wq->set_err(new_wq.err());

          for (bool finish = false; !finish;) {
            if (auto next_wq_it = std::ranges::find(
                    wqs_pending_, stripe_id,
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
        },
    };

    if (auto new_wq =
            wq->subquery(wb, chunk_sz, stripe_offset, new_wq_completer);
        !stripe_w_locker_.try_lock(stripe_id)) [[unlikely]] {
      wqs_pending_.emplace_back(stripe_id, std::move(new_wq));
    } else if (auto const res{process(stripe_id, std::move(new_wq))})
        [[unlikely]] {
      return res;
    }

    wb += chunk_sz;
  }

  return 0;
}

} // namespace ublk::raidsp
