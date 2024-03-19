#include "target.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>

#include <algorithm>
#include <iterator>
#include <memory>
#include <span>
#include <utility>
#include <vector>

#include <boost/dynamic_bitset/dynamic_bitset.hpp>
#include <boost/sml.hpp>

#include "mm/cache_line_aligned_allocator.hpp"
#include "mm/generic_allocators.hpp"
#include "mm/mem.hpp"
#include "mm/mem_chunk_pool.hpp"
#include "mm/mem_types.hpp"

#include "utils/algo.hpp"
#include "utils/bitset_locker.hpp"
#include "utils/math.hpp"
#include "utils/span.hpp"
#include "utils/utility.hpp"

#include "parity.hpp"
#include "read_query.hpp"
#include "rw_handler_interface.hpp"
#include "sector.hpp"
#include "write_query.hpp"

using namespace ublk;

namespace {

class rsp {
public:
  explicit rsp(uint64_t strip_sz, std::vector<std::shared_ptr<IRWHandler>> hs,
               std::function<uint64_t(uint64_t stripe_id)> const
                   &stripe_id_to_parity_id);
  ~rsp() = default;

  bool is_stripe_parity_coherent(uint64_t stripe_id) const noexcept;

  int process(std::shared_ptr<read_query> rq) noexcept;
  int process(std::shared_ptr<write_query> wq) noexcept;

private:
  std::vector<std::shared_ptr<IRWHandler>>
  handlers_generate(uint64_t stripe_id);

  int full_stripe_write_process(uint64_t stripe_id_at,
                                std::shared_ptr<write_query> wq) noexcept;

  constexpr static inline auto kCachedStripeAlignment = kSectorSz;
  static_assert(is_aligned_to(kCachedStripeAlignment,
                              alignof(std::max_align_t)));

  constexpr static inline auto kCachedParityAlignment = kSectorSz;
  static_assert(is_aligned_to(kCachedParityAlignment,
                              alignof(std::max_align_t)));

  template <typename T = std::byte>
  auto stripe_view(mm::uptrwd<std::byte[]> const &stripe) const noexcept {
    return to_span_of<T>(stripe_pool_->chunk_view(stripe));
  }

  template <typename T = std::byte>
  auto stripe_data_view(mm::uptrwd<std::byte[]> const &stripe) const noexcept {
    return to_span_of<T>(
        stripe_view(stripe).subspan(0, static_cfg_->stripe_data_sz));
  }

  template <typename T = std::byte>
  auto
  stripe_parity_view(mm::uptrwd<std::byte[]> const &stripe) const noexcept {
    return to_span_of<T>(
        stripe_view(stripe).subspan(static_cfg_->stripe_data_sz));
  }

  auto stripe_allocate() const noexcept { return stripe_pool_->get(); }

  auto stripe_parity_allocate() const noexcept {
    return stripe_parity_pool_->get();
  }

  int stripe_write(uint64_t stripe_id_at, std::shared_ptr<write_query> wqd,
                   std::shared_ptr<write_query> wqp) noexcept;
  int stripe_write(uint64_t stripe_id_at,
                   std::shared_ptr<write_query> wq) noexcept;

  int read_data_skip_parity(uint64_t stripe_id_from,
                            std::shared_ptr<read_query> rq) noexcept;
  int read_stripe_data(uint64_t stripe_id_from,
                       std::shared_ptr<read_query> rq) noexcept;
  int read_stripe_parity(uint64_t stripe_id_from,
                         std::shared_ptr<read_query> rq) noexcept;

  uint64_t stripe_id_to_parity_id(uint64_t stripe_id) const noexcept {
    return stripe_id_to_parity_id_(stripe_id);
  }

  int process(uint64_t stripe_id, std::shared_ptr<write_query> wq) noexcept;

  struct cfg_t {
    uint64_t strip_sz;
    uint64_t stripe_data_sz;
    uint64_t stripe_sz;
  };
  mm::uptrwd<cfg_t const> static_cfg_;

  std::vector<std::shared_ptr<IRWHandler>> hs_;

  std::function<uint64_t(uint64_t stripe_id)> stripe_id_to_parity_id_;

  bitset_locker<uint64_t, mm::allocator::cache_line_aligned_allocator<uint64_t>>
      stripe_w_locker_;

  mutable std::unique_ptr<mm::mem_chunk_pool> stripe_pool_;
  mutable std::unique_ptr<mm::mem_chunk_pool> stripe_parity_pool_;

  boost::dynamic_bitset<uint64_t> stripe_parity_coherency_state_;
  std::vector<std::pair<uint64_t, std::shared_ptr<write_query>>> wqs_pending_;
};

rsp::rsp(
    uint64_t strip_sz, std::vector<std::shared_ptr<IRWHandler>> hs,
    std::function<uint64_t(uint64_t stripe_id)> const &stripe_id_to_parity_id)
    : hs_(std::move(hs)), stripe_id_to_parity_id_(stripe_id_to_parity_id),
      stripe_w_locker_(0, mm::allocator::cache_line_aligned<uint64_t>::value) {
  assert(is_power_of_2(strip_sz));
  assert(is_multiple_of(strip_sz, kCachedStripeAlignment));
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

  stripe_pool_ = std::make_unique<mm::mem_chunk_pool>(kCachedStripeAlignment,
                                                      static_cfg_->stripe_sz);
  stripe_parity_pool_ = std::make_unique<mm::mem_chunk_pool>(
      kCachedParityAlignment, static_cfg_->strip_sz);
}

bool rsp::is_stripe_parity_coherent(uint64_t stripe_id) const noexcept {
  return stripe_id < stripe_parity_coherency_state_.size() &&
         stripe_parity_coherency_state_[stripe_id];
}

int rsp::read_data_skip_parity(uint64_t stripe_id_from,
                               std::shared_ptr<read_query> rq) noexcept {
  assert(!rq->buf().empty());
  assert(rq->offset() < static_cfg_->stripe_data_sz);

  auto stripe_id{stripe_id_from};
  auto stripe_offset{rq->offset()};

  for (size_t rb{0}; rb < rq->buf().size();) {
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

    stripe_offset = 0;
    ++stripe_id;
  }

  return 0;
}

int rsp::read_stripe_data(uint64_t stripe_id_from,
                          std::shared_ptr<read_query> rq) noexcept {
  assert(!rq->buf().empty());
  assert(!(rq->offset() + rq->buf().size() > static_cfg_->stripe_data_sz));

  return read_data_skip_parity(stripe_id_from, std::move(rq));
}

int rsp::read_stripe_parity(uint64_t stripe_id,
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
rsp::handlers_generate(uint64_t stripe_id) {
  std::vector<std::shared_ptr<IRWHandler>> hs;
  hs.reserve(hs_.size());

  auto const parity_id{stripe_id_to_parity_id(stripe_id)};
  assert(parity_id < hs_.size());

  std::copy_n(hs_.begin(), parity_id, std::back_inserter(hs));
  std::rotate_copy(hs_.begin() + parity_id, hs_.begin() + parity_id + 1,
                   hs_.end(), std::back_inserter(hs));

  return hs;
}

int rsp::stripe_write(uint64_t stripe_id_at, std::shared_ptr<write_query> wqd,
                      std::shared_ptr<write_query> wqp) noexcept {
  assert(wqd);
  assert(!wqd->buf().empty());
  assert(!(wqd->offset() + wqd->buf().size() > static_cfg_->stripe_data_sz));
  assert(wqp);
  assert(!wqp->buf().empty());
  assert(!(wqp->offset() + wqp->buf().size() > static_cfg_->strip_sz));

  auto combined_completer_guard{
      std::shared_ptr<std::nullptr_t>{
          nullptr,
          [=, this](std::nullptr_t *) {
            if (!wqd->err() && !wqp->err()) [[likely]] {
              assert(stripe_id_at < stripe_parity_coherency_state_.size());
              stripe_parity_coherency_state_.set(stripe_id_at);
            }
          },
      },
  };

  auto make_completer{
      [=](std::shared_ptr<write_query> wq) {
        return [wq, combined_completer_guard](write_query const &new_wq) {
          if (new_wq.err()) [[unlikely]] {
            wq->set_err(new_wq.err());
            return;
          }
        };
      },
  };

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
    auto new_wqd{wqd->subquery(wb, sq_sz, sq_off, make_completer(wqd))};
    if (auto const res{hs[hid]->submit(std::move(new_wqd))}) [[unlikely]] {
      wqd->set_err(res);
      return res;
    }
    wb += sq_sz;
  }
  assert(!(wb < wqd->buf().size()));

  auto new_wqp{
      wqp->subquery(0, wqp->buf().size(),
                    stripe_id_at * static_cfg_->strip_sz + wqp->offset(),
                    make_completer(wqp)),
  };

  if (auto const res{hs.back()->submit(std::move(new_wqp))}) [[unlikely]] {
    wqp->set_err(res);
    return res;
  }

  return 0;
}

int rsp::stripe_write(uint64_t stripe_id_at,
                      std::shared_ptr<write_query> wq) noexcept {
  assert(wq);
  assert(!wq->buf().empty());
  assert(!(wq->buf().size() < static_cfg_->stripe_sz));
  auto wqd{wq->subquery(0, static_cfg_->stripe_data_sz, 0, wq)};
  auto wqp{
      wq->subquery(static_cfg_->stripe_data_sz, static_cfg_->strip_sz, 0, wq)};
  return stripe_write(stripe_id_at, std::move(wqd), std::move(wqp));
}

int rsp::process(std::shared_ptr<read_query> rq) noexcept {
  assert(rq);
  assert(!rq->buf().empty());

  return read_data_skip_parity(
      rq->offset() / static_cfg_->stripe_data_sz,
      rq->subquery(0, rq->buf().size(),
                   rq->offset() % static_cfg_->stripe_data_sz, rq));
}

int rsp::full_stripe_write_process(uint64_t stripe_id_at,
                                   std::shared_ptr<write_query> wqd) noexcept {
  auto cached_stripe_parity{stripe_parity_allocate()};
  auto cached_stripe_parity_view{
      std::span{cached_stripe_parity.get(), static_cfg_->strip_sz},
  };

  /* Renew Parity of the stripe */
  parity_renew(wqd->buf(), cached_stripe_parity_view);

  auto wqp_completer{
      [wqd, cached_stripe_parity = std::shared_ptr{std::move(
                cached_stripe_parity)}](write_query const &new_wqp) {
        if (new_wqp.err()) [[unlikely]] {
          wqd->set_err(new_wqp.err());
          return;
        }
      },
  };

  auto wqp{
      write_query::create(cached_stripe_parity_view, 0,
                          std::move(wqp_completer)),
  };

  /*
   * Write Back the chunk including the newly incoming data
   * and the parity computed and updated
   */
  return stripe_write(stripe_id_at, std::move(wqd), std::move(wqp));
}

int rsp::process(uint64_t stripe_id, std::shared_ptr<write_query> wq) noexcept {
  assert(wq);
  assert(!wq->buf().empty());
  assert(!(wq->offset() + wq->buf().size() > static_cfg_->stripe_data_sz));

  /*
   * Read the whole stripe from the backend excluding parity in case we
   * intend to partially modify the stripe
   */
  if (wq->buf().size() < static_cfg_->stripe_data_sz) {
    auto new_cached_stripe{stripe_allocate()};
    auto new_cached_stripe_data_view{stripe_data_view(new_cached_stripe)};
    auto new_cached_stripe_parity_view{stripe_parity_view(new_cached_stripe)};

    auto new_rdq = std::shared_ptr<read_query>{};

    if (stripe_parity_coherency_state_[stripe_id]) [[likely]] {
      auto const new_cached_stripe_data_chunk{
          new_cached_stripe_data_view.subspan(wq->offset(), wq->buf().size()),
      };

      auto new_rdq_completer{
          [=, this,
           cached_stripe = std::shared_ptr{std::move(new_cached_stripe)}](
              read_query const &rdq) mutable {
            if (rdq.err()) [[unlikely]] {
              wq->set_err(rdq.err());
              return;
            }

            auto new_rpq_completer{
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

                  auto new_wqp{
                      write_query::create(
                          new_cached_stripe_parity_view, 0,
                          [wq, cached_stripe = std::move(cached_stripe)](
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
                          stripe_write(stripe_id, std::move(wq),
                                       std::move(new_wqp)),
                      }) [[unlikely]] {
                    return;
                  }
                },
            };

            auto new_rpq{
                read_query::create(new_cached_stripe_parity_view, 0,
                                   std::move(new_rpq_completer)),
            };

            if (auto const res{read_stripe_parity(
                    stripe_id, std::move(new_rpq))}) [[unlikely]] {
              wq->set_err(res);
              return;
            }
          },
      };

      new_rdq = read_query::create(new_cached_stripe_data_chunk, wq->offset(),
                                   std::move(new_rdq_completer));
    } else {
      auto new_rdq_completer{
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

            auto wqd_completer = [wq](write_query const &new_wq) {
              if (new_wq.err()) [[unlikely]] {
                wq->set_err(new_wq.err());
                return;
              }
            };

            auto new_wqd = write_query::create(chunk, wq->offset(),
                                               std::move(wqd_completer));

            auto wqp_completer =
                [wq, cached_stripe_sp = std::shared_ptr{
                         std::move(cached_stripe)}](write_query const &new_wq) {
                  if (new_wq.err()) [[unlikely]] {
                    wq->set_err(new_wq.err());
                    return;
                  }
                };

            auto new_wqp = write_query::create(new_cached_stripe_parity_view, 0,
                                               std::move(wqp_completer));

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
          },
      };

      new_rdq = read_query::create(new_cached_stripe_data_view, 0,
                                   std::move(new_rdq_completer));
    }

    if (auto const res{read_stripe_data(stripe_id, std::move(new_rdq))})
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

int rsp::process(std::shared_ptr<write_query> wq) noexcept {
  assert(wq);
  assert(!wq->buf().empty());

  stripe_w_locker_.extend(div_round_up(wq->offset() + wq->buf().size(),
                                       static_cfg_->stripe_data_sz));
  stripe_parity_coherency_state_.resize(stripe_w_locker_.size());

  auto stripe_id{wq->offset() / static_cfg_->stripe_data_sz};
  auto stripe_offset{wq->offset() % static_cfg_->stripe_data_sz};

  for (size_t wb{0}; wb < wq->buf().size(); ++stripe_id, stripe_offset = 0) {
    auto const chunk_sz{
        std::min(static_cfg_->stripe_data_sz - stripe_offset,
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

struct erq {
  std::shared_ptr<read_query> rq;
  mutable int r;
};

struct ewq {
  std::shared_ptr<write_query> wq;
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
