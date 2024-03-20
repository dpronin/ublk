#include "target.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>

#include <algorithm>
#include <memory>
#include <span>
#include <utility>
#include <vector>

#include <boost/dynamic_bitset/dynamic_bitset.hpp>
#include <boost/sml.hpp>

#include "mm/cache_line_aligned_allocator.hpp"
#include "mm/generic_allocators.hpp"
#include "mm/mem_chunk_pool.hpp"
#include "mm/mem_types.hpp"

#include "utils/algo.hpp"
#include "utils/bitset_locker.hpp"
#include "utils/math.hpp"
#include "utils/span.hpp"
#include "utils/utility.hpp"

#include "read_query.hpp"
#include "rw_handler_interface.hpp"
#include "write_query.hpp"

#include "backend.hpp"
#include "parity.hpp"

namespace {

class rsp {
public:
  explicit rsp(uint64_t strip_sz,
               std::vector<std::shared_ptr<ublk::IRWHandler>> hs,
               std::function<uint64_t(uint64_t stripe_id)> const
                   &stripe_id_to_parity_id);
  ~rsp() = default;

  bool is_stripe_parity_coherent(uint64_t stripe_id) const noexcept;

  int process(std::shared_ptr<ublk::read_query> rq) noexcept;
  int process(std::shared_ptr<ublk::write_query> wq) noexcept;

private:
  int stripe_write(uint64_t stripe_id_at,
                   std::shared_ptr<ublk::write_query> wqd,
                   std::shared_ptr<ublk::write_query> wqp) noexcept;

  int full_stripe_write_process(uint64_t stripe_id_at,
                                std::shared_ptr<ublk::write_query> wq) noexcept;

  constexpr static inline auto kCachedStripeAlignment =
      ublk::raidsp::backend::kAlignmentRequiredMin;
  static_assert(ublk::is_aligned_to(kCachedStripeAlignment,
                                    alignof(std::max_align_t)));

  constexpr static inline auto kCachedParityAlignment =
      ublk::raidsp::backend::kAlignmentRequiredMin;
  static_assert(ublk::is_aligned_to(kCachedParityAlignment,
                                    alignof(std::max_align_t)));

  template <typename T = std::byte>
  auto stripe_view(ublk::mm::uptrwd<std::byte[]> const &stripe) const noexcept {
    return ublk::to_span_of<T>(stripe_pool_->chunk_view(stripe));
  }

  template <typename T = std::byte>
  auto
  stripe_data_view(ublk::mm::uptrwd<std::byte[]> const &stripe) const noexcept {
    return ublk::to_span_of<T>(
        stripe_view(stripe).subspan(0, be_->static_cfg().stripe_data_sz));
  }

  template <typename T = std::byte>
  auto stripe_parity_view(
      ublk::mm::uptrwd<std::byte[]> const &stripe) const noexcept {
    return ublk::to_span_of<T>(
        stripe_view(stripe).subspan(be_->static_cfg().stripe_data_sz));
  }

  int process(uint64_t stripe_id,
              std::shared_ptr<ublk::write_query> wq) noexcept;

  std::unique_ptr<ublk::raidsp::backend> be_;
  ublk::bitset_locker<
      uint64_t, ublk::mm::allocator::cache_line_aligned_allocator<uint64_t>>
      stripe_w_locker_;
  std::unique_ptr<ublk::mm::mem_chunk_pool> stripe_pool_;
  std::unique_ptr<ublk::mm::mem_chunk_pool> stripe_parity_pool_;

  boost::dynamic_bitset<uint64_t> stripe_parity_coherency_state_;
  std::vector<std::pair<uint64_t, std::shared_ptr<ublk::write_query>>>
      wqs_pending_;
};

rsp::rsp(
    uint64_t strip_sz, std::vector<std::shared_ptr<ublk::IRWHandler>> hs,
    std::function<uint64_t(uint64_t stripe_id)> const &stripe_id_to_parity_id)
    : be_(std::make_unique<ublk::raidsp::backend>(strip_sz, std::move(hs),
                                                  stripe_id_to_parity_id)),
      stripe_w_locker_(
          0, ublk::mm::allocator::cache_line_aligned<uint64_t>::value),
      stripe_pool_(std::make_unique<ublk::mm::mem_chunk_pool>(
          kCachedStripeAlignment, be_->static_cfg().stripe_sz)),
      stripe_parity_pool_(std::make_unique<ublk::mm::mem_chunk_pool>(
          kCachedParityAlignment, be_->static_cfg().strip_sz)) {}

bool rsp::is_stripe_parity_coherent(uint64_t stripe_id) const noexcept {
  return stripe_id < stripe_parity_coherency_state_.size() &&
         stripe_parity_coherency_state_[stripe_id];
}

int rsp::process(std::shared_ptr<ublk::read_query> rq) noexcept {
  assert(rq);

  return be_->data_skip_parity_read(
      rq->offset() / be_->static_cfg().stripe_data_sz,
      rq->subquery(0, rq->buf().size(),
                   rq->offset() % be_->static_cfg().stripe_data_sz, rq));
}

int rsp::stripe_write(uint64_t stripe_id_at,
                      std::shared_ptr<ublk::write_query> wqd,
                      std::shared_ptr<ublk::write_query> wqp) noexcept {
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
          std::shared_ptr<ublk::write_query> wq) {
        return [wq = std::move(wq),
                combined_completer_guard](ublk::write_query const &new_wq) {
          if (new_wq.err()) [[unlikely]] {
            wq->set_err(new_wq.err());
            return;
          }
        };
      },
  };

  auto new_wqd{
      ublk::write_query::create(wqd->buf(), wqd->offset(),
                                make_a_new_completer(wqd)),
  };

  auto new_wqp{
      ublk::write_query::create(wqp->buf(), wqp->offset(),
                                make_a_new_completer(wqp)),
  };

  return be_->stripe_write(stripe_id_at, std::move(wqd), std::move(wqp));
}

int rsp::full_stripe_write_process(
    uint64_t stripe_id_at, std::shared_ptr<ublk::write_query> wqd) noexcept {
  auto cached_stripe_parity{stripe_parity_pool_->get()};
  auto cached_stripe_parity_view{
      std::span{cached_stripe_parity.get(), be_->static_cfg().strip_sz},
  };

  /* Renew Parity of the stripe */
  ublk::parity_renew(wqd->buf(), cached_stripe_parity_view);

  auto wqp_completer{
      [wqd, cached_stripe_parity = std::shared_ptr{std::move(
                cached_stripe_parity)}](ublk::write_query const &new_wqp) {
        if (new_wqp.err()) [[unlikely]] {
          wqd->set_err(new_wqp.err());
          return;
        }
      },
  };

  auto wqp{
      ublk::write_query::create(cached_stripe_parity_view, 0,
                                std::move(wqp_completer)),
  };

  /*
   * Write Back the chunk including the newly incoming data
   * and the parity computed and updated
   */
  return stripe_write(stripe_id_at, std::move(wqd), std::move(wqp));
}

int rsp::process(uint64_t stripe_id,
                 std::shared_ptr<ublk::write_query> wq) noexcept {
  assert(wq);
  assert(!wq->buf().empty());
  assert(!(wq->offset() + wq->buf().size() > be_->static_cfg().stripe_data_sz));

  /*
   * Read the whole stripe from the backend excluding parity in case we
   * intend to partially modify the stripe
   */
  if (wq->buf().size() < be_->static_cfg().stripe_data_sz) {
    auto new_cached_stripe{stripe_pool_->get()};
    auto new_cached_stripe_data_view{stripe_data_view(new_cached_stripe)};
    auto new_cached_stripe_parity_view{stripe_parity_view(new_cached_stripe)};

    auto new_rdq{std::shared_ptr<ublk::read_query>{}};

    if (stripe_parity_coherency_state_[stripe_id]) [[likely]] {
      new_cached_stripe_data_view =
          new_cached_stripe_data_view.subspan(wq->offset(), wq->buf().size());

      auto new_rdq_completer{
          [=, this,
           cached_stripe = std::shared_ptr{std::move(new_cached_stripe)}](
              ublk::read_query const &rdq) mutable {
            if (rdq.err()) [[unlikely]] {
              wq->set_err(rdq.err());
              return;
            }

            auto new_rpq_completer{
                [=, this, cached_stripe = std::move(cached_stripe)](
                    ublk::read_query const &rpq) mutable {
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
                  ublk::math::xor_to(wq->buf(), new_cached_stripe_data_view);
                  ublk::parity_to(new_cached_stripe_data_view,
                                  new_cached_stripe_parity_view,
                                  wq->offset() %
                                      new_cached_stripe_parity_view.size());

                  auto wqp{
                      ublk::write_query::create(
                          new_cached_stripe_parity_view, 0,
                          [wq, cached_stripe = std::move(cached_stripe)](
                              ublk::write_query const &new_wqp) {
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
                          stripe_write(stripe_id, std::move(wq),
                                       std::move(wqp)),
                      }) [[unlikely]] {
                    return;
                  }
                },
            };

            auto new_rpq{
                ublk::read_query::create(new_cached_stripe_parity_view, 0,
                                         std::move(new_rpq_completer)),
            };

            if (auto const res{
                    be_->stripe_parity_read(stripe_id, std::move(new_rpq)),
                }) [[unlikely]] {
              wq->set_err(res);
              return;
            }
          },
      };

      new_rdq =
          ublk::read_query::create(new_cached_stripe_data_view, wq->offset(),
                                   std::move(new_rdq_completer));
    } else {
      auto new_rdq_completer{
          [=, this,
           cached_stripe = std::shared_ptr{std::move(new_cached_stripe)}](
              ublk::read_query const &rdq) mutable {
            if (rdq.err()) [[unlikely]] {
              wq->set_err(rdq.err());
              return;
            }

            auto const chunk{wq->buf()};

            auto const copy_from{chunk};
            auto const copy_to{
                new_cached_stripe_data_view.subspan(wq->offset(), chunk.size()),
            };

            /* Modify the part of the stripe with the new data come in */
            ublk::algo::copy(copy_from, copy_to);

            /* Renew Parity of the stripe */
            ublk::parity_renew(new_cached_stripe_data_view,
                               new_cached_stripe_parity_view);

            auto wqd_completer{
                [wq](ublk::write_query const &new_wq) {
                  if (new_wq.err()) [[unlikely]] {
                    wq->set_err(new_wq.err());
                    return;
                  }
                },
            };

            auto new_wqd{
                ublk::write_query::create(chunk, wq->offset(),
                                          std::move(wqd_completer)),
            };

            auto wqp_completer{
                [wq, cached_stripe_sp = std::shared_ptr{std::move(
                         cached_stripe)}](ublk::write_query const &new_wq) {
                  if (new_wq.err()) [[unlikely]] {
                    wq->set_err(new_wq.err());
                    return;
                  }
                },
            };

            auto new_wqp{
                ublk::write_query::create(new_cached_stripe_parity_view, 0,
                                          std::move(wqp_completer)),
            };

            /*
             * Write Back the chunk including the newly incoming data
             * and the parity computed and updated
             */
            if (auto const res{
                    stripe_write(stripe_id, std::move(new_wqd),
                                 std::move(new_wqp)),
                }) [[unlikely]] {
              wq->set_err(res);
              return;
            }
          },
      };

      new_rdq = ublk::read_query::create(new_cached_stripe_data_view, 0,
                                         std::move(new_rdq_completer));
    }

    if (auto const res{be_->stripe_data_read(stripe_id, std::move(new_rdq))})
        [[unlikely]] {
      return res;
    }

    /*
     * Calculate parity based on newly incoming stripe-long chunk and write
     * back the whole stripe including the chunk and parity computed
     */
  } else if (auto const res{
                 full_stripe_write_process(stripe_id, std::move(wq)),
             }) {
    return res;
  }

  return 0;
}

int rsp::process(std::shared_ptr<ublk::write_query> wq) noexcept {
  assert(wq);
  assert(!wq->buf().empty());

  stripe_w_locker_.extend(ublk::div_round_up(wq->offset() + wq->buf().size(),
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
        [wq, stripe_id, this](ublk::write_query const &new_wq) {
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

struct erq {
  std::shared_ptr<ublk::read_query> rq;
  mutable int r;
};

struct ewq {
  std::shared_ptr<ublk::write_query> wq;
  mutable int r;
};

struct estripecohcheck {
  uint64_t stripe_id;
  mutable bool r;
};

/* clang-format off */
struct transition_table {
  auto operator()() noexcept {
    using namespace boost::sml;
    return make_transition_table(
       // online state
       *"online"_s + event<erq> [ ([](erq const &e, rsp& r){ e.r = r.process(std::move(e.rq)); return 0 == e.r; }) ]
      , "online"_s + event<erq> = "offline"_s
      , "online"_s + event<ewq> [ ([](ewq const &e, rsp& r){ e.r = r.process(std::move(e.wq)); return 0 == e.r; }) ]
      , "online"_s + event<ewq> = "offline"_s
      , "online"_s + event<estripecohcheck> / [](estripecohcheck const &e, rsp const& r) { e.r = r.is_stripe_parity_coherent(e.stripe_id); }
       // offline state
      , "offline"_s + event<erq> / [](erq const &e) { e.r = EIO; }
      , "offline"_s + event<ewq> / [](ewq const &e) { e.r = EIO; }
      , "offline"_s + event<estripecohcheck> / [](estripecohcheck const &e) { e.r = false; }
    );
  }
};
/* clang-format on */

} // namespace

namespace ublk::raidsp {

class Target::impl final {
public:
  explicit impl(
      uint64_t strip_sz, std::vector<std::shared_ptr<IRWHandler>> hs,
      std::function<uint64_t(uint64_t stripe_id)> const &stripe_id_to_parity_id)
      : rsp_(strip_sz, std::move(hs), stripe_id_to_parity_id), fsm_(rsp_) {}

  int process(std::shared_ptr<read_query> rq) noexcept {
    erq e{.rq = std::move(rq), .r = 0};
    fsm_.process_event(e);
    return e.r;
  }

  int process(std::shared_ptr<write_query> wq) noexcept {
    ewq e{.wq = std::move(wq), .r = 0};
    fsm_.process_event(e);
    return e.r;
  }

  bool is_stripe_parity_coherent(uint64_t stripe_id) const noexcept {
    estripecohcheck e{.stripe_id = stripe_id, .r = false};
    fsm_.process_event(e);
    return e.r;
  }

private:
  rsp rsp_;
  mutable boost::sml::sm<transition_table> fsm_;
};

Target::Target(
    uint64_t strip_sz, std::vector<std::shared_ptr<IRWHandler>> hs,
    std::function<uint64_t(uint64_t stripe_id)> const &stripe_id_to_parity_id)
    : pimpl_(std::make_unique<impl>(strip_sz, std::move(hs),
                                    stripe_id_to_parity_id)) {}

Target::~Target() noexcept = default;

Target::Target(Target &&) noexcept = default;
Target &Target::operator=(Target &&) noexcept = default;

bool Target::is_stripe_parity_coherent(uint64_t stripe_id) const noexcept {
  return pimpl_->is_stripe_parity_coherent(stripe_id);
}

int Target::process(std::shared_ptr<read_query> rq) noexcept {
  return pimpl_->process(std::move(rq));
}

int Target::process(std::shared_ptr<write_query> wq) noexcept {
  return pimpl_->process(std::move(wq));
}

} // namespace ublk::raidsp
