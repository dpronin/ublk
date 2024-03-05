#include "cached_rw_handler.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>

#include <algorithm>
#include <functional>
#include <memory>
#include <span>
#include <stack>
#include <utility>

#include <boost/dynamic_bitset/dynamic_bitset.hpp>

#include "mm/generic_allocators.hpp"
#include "mm/mem.hpp"
#include "mm/mem_types.hpp"

#include "utils/algo.hpp"
#include "utils/span.hpp"
#include "utils/utility.hpp"

#include "read_query.hpp"
#include "rw_handler_interface.hpp"
#include "sector.hpp"
#include "write_query.hpp"

namespace {

class bitset_rw_semaphore final {
private:
  using state_t = boost::dynamic_bitset<
      uint64_t, ublk::mm::allocator::cache_line_aligned_allocator<uint64_t>>;

  void extend(uint64_t nr, state_t &lock_state) {
    if (!(nr < lock_state.size()))
      lock_state.resize(nr + 1);
  }

public:
  explicit bitset_rw_semaphore(uint64_t preallocate_size = 0)
      : rlock_state_(preallocate_size, 0,
                     ublk::mm::allocator::cache_line_aligned<uint64_t>::value),
        wlock_state_(preallocate_size, 0,
                     ublk::mm::allocator::cache_line_aligned<uint64_t>::value) {
  }
  ~bitset_rw_semaphore() = default;

  bitset_rw_semaphore(bitset_rw_semaphore const &) = delete;
  bitset_rw_semaphore &operator=(bitset_rw_semaphore const &) = delete;

  bitset_rw_semaphore(bitset_rw_semaphore &&) = delete;
  bitset_rw_semaphore &operator=(bitset_rw_semaphore &&) = delete;

  void extend(uint64_t nr) noexcept {
    for (auto *p_lock_state : {&rlock_state_, &wlock_state_})
      extend(nr, *p_lock_state);
  }

  bool try_read_lock(uint64_t pos) noexcept {
    assert(pos < rlock_state_.size());
    return !rlock_state_.test_set(pos);
  }

  void read_unlock(uint64_t pos) noexcept {
    assert(pos < rlock_state_.size());
    assert(rlock_state_[pos]);
    rlock_state_.reset(pos);
  }

  bool try_write_lock(uint64_t pos) noexcept {
    assert(pos < wlock_state_.size());
    return !wlock_state_.test_set(pos);
  }

  void write_unlock(uint64_t pos) noexcept {
    assert(pos < wlock_state_.size());
    assert(wlock_state_[pos]);
    wlock_state_.reset(pos);
  }

private:
  state_t rlock_state_, wlock_state_;
};

class mem_chunk_pool final {
public:
  explicit mem_chunk_pool(
      std::function<ublk::mm::uptrwd<std::byte[]>()> generator,
      size_t alignment, size_t chunk_sz) noexcept
      : generator_(std::move(generator)), alignment_(alignment),
        chunk_sz_(chunk_sz) {
    assert(generator_);
    assert(ublk::is_power_of_2(alignment_));
  }
  ~mem_chunk_pool() = default;

  mem_chunk_pool(mem_chunk_pool const &) = delete;
  mem_chunk_pool &operator=(mem_chunk_pool const &) = delete;

  mem_chunk_pool(mem_chunk_pool &&) = delete;
  mem_chunk_pool &operator=(mem_chunk_pool &&) = delete;

  size_t chunk_alignment() const noexcept { return alignment_; }
  size_t chunk_sz() const noexcept { return chunk_sz_; }

  template <typename T = std::byte>
  auto chunk_view(ublk::mm::uptrwd<std::byte[]> const &mem_chunk) noexcept {
    return ublk::to_span_of<T>({mem_chunk.get(), chunk_sz_});
  }

  ublk::mm::uptrwd<std::byte[]> get() noexcept {
    auto chunk = ublk::mm::uptrwd<std::byte[]>{};
    if (!free_chunks_.empty()) {
      chunk = std::move(free_chunks_.top());
      free_chunks_.pop();
    } else {
      chunk = generator_();
    }
    return {
        chunk.release(),
        [this, d = std::move(chunk.get_deleter())](auto *p) mutable {
          free_chunks_.push({p, std::move(d)});
        },
    };
  }

private:
  std::function<ublk::mm::uptrwd<std::byte[]>()> generator_;
  size_t alignment_;
  size_t chunk_sz_;
  std::stack<ublk::mm::uptrwd<std::byte[]>> free_chunks_;
};

class CachedRWIHandler : public ublk::IRWHandler {
public:
  explicit CachedRWIHandler(
      std::shared_ptr<ublk::flat_lru_cache<uint64_t, std::byte>> cache,
      std::shared_ptr<ublk::IRWHandler> handler,
      std::shared_ptr<mem_chunk_pool> mem_chunk_pool) noexcept;
  ~CachedRWIHandler() override = default;

  CachedRWIHandler(CachedRWIHandler const &) = delete;
  CachedRWIHandler &operator=(CachedRWIHandler const &) = delete;

  CachedRWIHandler(CachedRWIHandler &&) = delete;
  CachedRWIHandler &operator=(CachedRWIHandler &&) = delete;

  int submit(std::shared_ptr<ublk::read_query> rq) noexcept override;
  int submit(std::shared_ptr<ublk::write_query> wq) noexcept override;

protected:
  std::shared_ptr<ublk::flat_lru_cache<uint64_t, std::byte>> cache_;
  std::shared_ptr<ublk::IRWHandler> handler_;
  std::shared_ptr<mem_chunk_pool> mem_chunk_pool_;

  struct pending_rq {
    uint64_t chunk_id;
    uint64_t chunk_offset;
    std::span<std::byte> chunk;
    std::shared_ptr<ublk::read_query> rq;
  };
  std::vector<pending_rq> rqs_pending_;

  bitset_rw_semaphore chunk_rw_semaphore_;
};

CachedRWIHandler::CachedRWIHandler(
    std::shared_ptr<ublk::flat_lru_cache<uint64_t, std::byte>> cache,
    std::shared_ptr<ublk::IRWHandler> handler,
    std::shared_ptr<mem_chunk_pool> mem_chunk_pool) noexcept
    : cache_(std::move(cache)), handler_(std::move(handler)),
      mem_chunk_pool_(std::move(mem_chunk_pool)) {
  assert(cache_);
  assert(handler_);
  assert(mem_chunk_pool_);
  chunk_rw_semaphore_.extend(cache_->len());
}

int CachedRWIHandler::submit(std::shared_ptr<ublk::read_query> rq) noexcept {
  assert(rq);
  assert(0 != rq->buf().size());

  chunk_rw_semaphore_.extend(
      ublk::div_round_up(rq->offset() + rq->buf().size(), cache_->item_sz()) -
      1);

  auto chunk_id{rq->offset() / cache_->item_sz()};
  auto chunk_offset{rq->offset() % cache_->item_sz()};

  for (size_t rb{0}; rb < rq->buf().size();) {
    auto const chunk_sz{
        std::min(cache_->item_sz() - chunk_offset, rq->buf().size() - rb),
    };
    auto const chunk{rq->buf().subspan(rb, chunk_sz)};

    if (auto cached_chunk = cache_->find(chunk_id); !cached_chunk.empty())
        [[likely]] {
      auto const from{cached_chunk.subspan(chunk_offset, chunk.size())};
      auto const to{chunk};
      ublk::algo::copy(from, to);
    } else if (chunk_rw_semaphore_.try_read_lock(chunk_id)) [[likely]] {
      auto mem_chunk = mem_chunk_pool_->get();
      assert(mem_chunk);

      auto mem_chunk_span = mem_chunk_pool_->chunk_view(mem_chunk);

      auto chunk_rq = ublk::read_query::create(
          mem_chunk_span, chunk_id * cache_->item_sz(),
          [=, this,
           mem_chunk_holder = std::make_shared<decltype(mem_chunk)>(
               std::move(mem_chunk))](ublk::read_query const &new_rq) mutable {
            if (new_rq.err()) [[unlikely]] {
              rq->set_err(new_rq.err());
              return;
            }

            auto const from{new_rq.buf().subspan(chunk_offset, chunk.size())};
            auto const to{chunk};
            ublk::algo::copy(from, to);

            if (!cache_->exists(chunk_id))
              cache_->update({chunk_id, std::move(*mem_chunk_holder.get())});

            auto rq_it =
                std::partition(rqs_pending_.begin(), rqs_pending_.end(),
                               [chunk_id](auto const &rq_pend) {
                                 return chunk_id != rq_pend.chunk_id;
                               });
            for (auto it = rq_it; it != rqs_pending_.end(); ++it) {
              auto const from{
                  new_rq.buf().subspan(it->chunk_offset, it->chunk.size())};
              auto const to{it->chunk};
              ublk::algo::copy(from, to);
            }

            rqs_pending_.erase(rq_it, rqs_pending_.end());

            chunk_rw_semaphore_.read_unlock(chunk_id);
          });
      if (auto const res{handler_->submit(std::move(chunk_rq))}) [[unlikely]] {
        return res;
      }
    } else {
      rqs_pending_.emplace_back(chunk_id, chunk_offset, chunk, rq);
    }

    ++chunk_id;
    chunk_offset = 0;
    rb += chunk.size();
  }

  return 0;
}

int CachedRWIHandler::submit(std::shared_ptr<ublk::write_query> wq) noexcept {
  assert(wq);
  assert(0 != wq->buf().size());

  auto const chunk_id_first{wq->offset() / cache_->item_sz()};
  auto const chunk_id_last{
      ublk::div_round_up(wq->offset() + wq->buf().size(), cache_->item_sz()) -
          1,
  };

  auto new_wq{
      wq->subquery(0, wq->buf().size(), wq->offset(),
                   [this, chunk_id_first, chunk_id_last,
                    wq](ublk::write_query const &chunk_wq) {
                     cache_->invalidate_range({chunk_id_first, chunk_id_last});
                     if (chunk_wq.err()) [[unlikely]] {
                       wq->set_err(chunk_wq.err());
                       return;
                     }
                   }),
  };

  if (auto const res{handler_->submit(std::move(new_wq))}) [[unlikely]] {
    cache_->invalidate_range({chunk_id_first, chunk_id_last});
    return res;
  }

  return 0;
}

class CachedRWTHandler : public CachedRWIHandler {
public:
  using CachedRWIHandler::CachedRWIHandler;
  ~CachedRWTHandler() override = default;

  CachedRWTHandler(CachedRWTHandler const &) = delete;
  CachedRWTHandler &operator=(CachedRWTHandler const &) = delete;

  CachedRWTHandler(CachedRWTHandler &&) = delete;
  CachedRWTHandler &operator=(CachedRWTHandler &&) = delete;

  int submit(std::shared_ptr<ublk::write_query> wq) noexcept override;

private:
  int process(std::shared_ptr<ublk::write_query> wq) noexcept;

  std::vector<std::pair<uint64_t, std::shared_ptr<ublk::write_query>>>
      wqs_pending_;
};

int CachedRWTHandler::process(std::shared_ptr<ublk::write_query> wq) noexcept {
  assert(wq);

  auto chunk_id{wq->offset() / cache_->item_sz()};
  auto chunk_offset{wq->offset() % cache_->item_sz()};

  if (auto const chunk{wq->buf()}; !(chunk.size() < cache_->item_sz())) {
    assert(chunk.size() == cache_->item_sz());
    assert(0 == chunk_offset);

    auto mem_chunk = mem_chunk_pool_->get();
    assert(mem_chunk);

    ublk::algo::copy(chunk, mem_chunk_pool_->chunk_view(mem_chunk));

    cache_->update({chunk_id, std::move(mem_chunk)});
  } else if (auto cached_chunk = cache_->find_mutable(chunk_id);
             !cached_chunk.empty()) {
    assert(!(chunk_offset + chunk.size() > cached_chunk.size()));
    auto const from{chunk};
    auto const to{cached_chunk.subspan(chunk_offset, chunk.size())};
    ublk::algo::copy(from, to);
  } else {
    auto mem_chunk = mem_chunk_pool_->get();
    assert(mem_chunk);

    auto mem_chunk_span = mem_chunk_pool_->chunk_view(mem_chunk);
    auto rmwq = ublk::read_query::create(
        mem_chunk_span, chunk_id * cache_->item_sz(),
        [this, chunk_id, chunk_offset, chunk, mem_chunk_span,
         wq = std::move(wq),
         mem_chunk_holder = std::make_shared<decltype(mem_chunk)>(
             std::move(mem_chunk))](ublk::read_query const &rmwq) mutable {
          if (rmwq.err()) [[unlikely]] {
            wq->set_err(rmwq.err());
            return;
          }

          auto const from{chunk};
          auto const to{mem_chunk_span.subspan(chunk_offset, chunk.size())};
          ublk::algo::copy(from, to);

          cache_->update({chunk_id, std::move(*mem_chunk_holder.get())});

          if (auto const res{handler_->submit(std::move(wq))}) [[unlikely]] {
            cache_->invalidate(chunk_id);
          }
        });
    if (auto const res{handler_->submit(std::move(rmwq))}) [[unlikely]] {
      return res;
    }
  }

  if (wq) {
    if (auto const res{handler_->submit(std::move(wq))}) [[unlikely]] {
      cache_->invalidate(chunk_id);
      return res;
    }
  }

  return 0;
}

int CachedRWTHandler::submit(std::shared_ptr<ublk::write_query> wq) noexcept {
  assert(wq);
  assert(0 != wq->buf().size());

  chunk_rw_semaphore_.extend(
      ublk::div_round_up(wq->offset() + wq->buf().size(), cache_->item_sz()) -
      1);

  auto chunk_id{wq->offset() / cache_->item_sz()};
  auto chunk_offset{wq->offset() % cache_->item_sz()};

  for (size_t wb{0}; wb < wq->buf().size();) {
    auto const chunk_sz{
        std::min(cache_->item_sz() - chunk_offset, wq->buf().size() - wb),
    };

    auto chunk_wq{
        wq->subquery(
            wb, chunk_sz, wq->offset() + wb,
            [this, chunk_id, wq](ublk::write_query const &chunk_wq) {
              if (chunk_wq.err()) [[unlikely]] {
                cache_->invalidate(chunk_id);
                wq->set_err(chunk_wq.err());
                return;
              }

              for (bool finish = false; !finish;) {
                if (auto next_wq_it = std::ranges::find_if(
                        wqs_pending_,
                        [chunk_id](uint64_t wq_chunk_id) {
                          return chunk_id == wq_chunk_id;
                        },
                        [](auto const &wq_pend) { return wq_pend.first; });
                    next_wq_it != wqs_pending_.end()) {
                  finish = 0 == process(std::move(next_wq_it->second));
                  std::iter_swap(next_wq_it, wqs_pending_.end() - 1);
                  wqs_pending_.pop_back();
                } else {
                  chunk_rw_semaphore_.write_unlock(chunk_id);
                  finish = true;
                }
              }
            }),
    };

    if (!chunk_rw_semaphore_.try_write_lock(chunk_id)) [[unlikely]] {
      wqs_pending_.emplace_back(chunk_id, std::move(chunk_wq));
    } else if (auto const res{process(std::move(chunk_wq))}) {
      return res;
    }

    ++chunk_id;
    chunk_offset = 0;
    wb += chunk_sz;
  }

  return 0;
}

} // namespace

namespace ublk {

CachedRWHandler::CachedRWHandler(
    std::unique_ptr<flat_lru_cache<uint64_t, std::byte>> cache,
    std::unique_ptr<IRWHandler> handler, bool write_through /* = true*/) {
  assert(!(cache->item_sz() < kSectorSz));
  assert(is_power_of_2(cache->item_sz()));

  auto cache_sp{std::shared_ptr{std::move(cache)}};
  auto handler_sp{std::shared_ptr{std::move(handler)}};
  auto pool = std::make_shared<mem_chunk_pool>(
      mm::get_unique_bytes_generator(kCachedChunkAlignment,
                                     cache_sp->item_sz()),
      kCachedChunkAlignment, cache_sp->item_sz());

  handlers_[0] = std::make_shared<CachedRWIHandler>(cache_sp, handler_sp, pool);
  handlers_[1] = std::make_shared<CachedRWTHandler>(cache_sp, handler_sp, pool);

  set_write_through(write_through);
}

void CachedRWHandler::set_write_through(bool value) noexcept {
  handler_ = handlers_[!!value];
}

bool CachedRWHandler::write_through() const noexcept {
  return handler_ == handlers_[1];
}

int CachedRWHandler::submit(std::shared_ptr<read_query> rq) noexcept {
  return handler_->submit(std::move(rq));
}

int CachedRWHandler::submit(std::shared_ptr<write_query> wq) noexcept {
  return handler_->submit(std::move(wq));
}

} // namespace ublk
