#include "cached_rw_handler.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>

#include <functional>
#include <memory>
#include <span>
#include <stack>
#include <utility>

#include <boost/dynamic_bitset/dynamic_bitset.hpp>

#include "mm/mem.hpp"
#include "mm/mem_types.hpp"

#include "algo.hpp"
#include "read_query.hpp"
#include "rw_handler_interface.hpp"
#include "sector.hpp"
#include "span.hpp"
#include "write_query.hpp"

namespace {

class mem_chunk_pool {
public:
  explicit mem_chunk_pool(
      std::function<ublk::mm::uptrwd<std::byte[]>()> generator) noexcept
      : generator_(std::move(generator)) {
    assert(generator_);
  }
  ~mem_chunk_pool() = default;

  mem_chunk_pool(mem_chunk_pool const &) = delete;
  mem_chunk_pool &operator=(mem_chunk_pool const &) = delete;

  mem_chunk_pool(mem_chunk_pool &&) = delete;
  mem_chunk_pool &operator=(mem_chunk_pool &&) = delete;

  ublk::mm::uptrwd<std::byte[]> get() noexcept {
    auto chunk = ublk::mm::uptrwd<std::byte[]>{};
    if (!free_chunks_.empty()) {
      chunk = std::move(free_chunks_.top());
      free_chunks_.pop();
    } else {
      chunk = generator_();
    }
    return chunk;
  }

  void put(ublk::mm::uptrwd<std::byte[]> &&chunk) noexcept {
    free_chunks_.push(std::move(chunk));
  }

private:
  std::function<ublk::mm::uptrwd<std::byte[]>()> generator_;
  std::stack<ublk::mm::uptrwd<std::byte[]>> free_chunks_;
};

class CachedRWHandlerBase : public ublk::IRWHandler {
public:
  CachedRWHandlerBase(
      std::shared_ptr<ublk::flat_lru_cache<uint64_t, std::byte>> cache,
      std::shared_ptr<ublk::IRWHandler> handler,
      std::shared_ptr<mem_chunk_pool> mem_chunk_pool) noexcept
      : cache_(std::move(cache)), handler_(std::move(handler)),
        mem_chunk_pool_(std::move(mem_chunk_pool)) {
    assert(cache_);
    assert(handler_);
    assert(mem_chunk_pool_);
  }
  ~CachedRWHandlerBase() override = default;

  CachedRWHandlerBase(CachedRWHandlerBase const &) = delete;
  CachedRWHandlerBase &operator=(CachedRWHandlerBase const &) = delete;

  CachedRWHandlerBase(CachedRWHandlerBase &&) = delete;
  CachedRWHandlerBase &operator=(CachedRWHandlerBase &&) = delete;

  int submit(std::shared_ptr<ublk::read_query> rq) noexcept override;
  int submit(std::shared_ptr<ublk::write_query> rq) noexcept override = 0;

protected:
  template <typename T = std::byte>
  auto mem_chunk_view(ublk::mm::uptrwd<std::byte[]> const &mem_chunk) noexcept {
    return ublk::to_span_of<T>({mem_chunk.get(), cache_->item_sz()});
  }

  void
  cache_full_line_update(uint64_t chunk_id,
                         ublk::mm::uptrwd<std::byte[]> &&mem_chunk) noexcept {
    if (auto evicted_value{cache_->update({chunk_id, std::move(mem_chunk)})})
        [[likely]] {
      assert(evicted_value->second);
      mem_chunk_pool_->put(std::move(evicted_value->second));
    }
  }

  std::shared_ptr<ublk::flat_lru_cache<uint64_t, std::byte>> cache_;
  std::shared_ptr<ublk::IRWHandler> handler_;
  std::shared_ptr<mem_chunk_pool> mem_chunk_pool_;
};

int CachedRWHandlerBase::submit(std::shared_ptr<ublk::read_query> rq) noexcept {
  assert(rq);

  auto chunk_id{rq->offset() / cache_->item_sz()};
  auto chunk_offset{rq->offset() % cache_->item_sz()};

  for (size_t rb{0}; rb < rq->buf().size();) {
    auto const chunk_sz{
        std::min(cache_->item_sz() - chunk_offset, rq->buf().size() - rb),
    };
    auto const chunk{rq->buf().subspan(rb, chunk_sz)};

    if (auto cached_chunk = cache_->find(chunk_id); !cached_chunk.empty()) {
      auto const from{cached_chunk.subspan(chunk_offset, chunk.size())};
      auto const to{chunk};
      ublk::algo::copy(from, to);
    } else {
      auto mem_chunk = mem_chunk_pool_->get();
      assert(mem_chunk);

      auto mem_chunk_span = mem_chunk_view(mem_chunk);
      auto new_rq = ublk::read_query::create(
          mem_chunk_span, chunk_id * cache_->item_sz(),
          [this, chunk_id, chunk_offset, chunk, rq,
           mem_chunk_holder = std::make_shared<decltype(mem_chunk)>(
               std::move(mem_chunk))](ublk::read_query const &new_rq) mutable {
            if (new_rq.err()) [[unlikely]] {
              rq->set_err(new_rq.err());
              return;
            }

            auto const from{new_rq.buf().subspan(chunk_offset, chunk.size())};
            auto const to{chunk};
            ublk::algo::copy(from, to);

            if (!cache_->exists(chunk_id)) {
              cache_full_line_update(chunk_id,
                                     std::move(*mem_chunk_holder.get()));
            }
          });
      if (auto const res{handler_->submit(std::move(new_rq))}) [[unlikely]] {
        return res;
      }
    }

    ++chunk_id;
    chunk_offset = 0;
    rb += chunk.size();
  }

  return 0;
}

class CachedRWIHandler : public CachedRWHandlerBase {
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

  int submit(std::shared_ptr<ublk::write_query> wq) noexcept override;
};

CachedRWIHandler::CachedRWIHandler(
    std::shared_ptr<ublk::flat_lru_cache<uint64_t, std::byte>> cache,
    std::shared_ptr<ublk::IRWHandler> handler,
    std::shared_ptr<mem_chunk_pool> mem_chunk_pool) noexcept
    : CachedRWHandlerBase(std::move(cache), std::move(handler),
                          std::move(mem_chunk_pool)) {}

int CachedRWIHandler::submit(std::shared_ptr<ublk::write_query> wq) noexcept {
  assert(wq);

  auto chunk_id{wq->offset() / cache_->item_sz()};
  auto chunk_offset{wq->offset() % cache_->item_sz()};

  for (size_t wb{0}; wb < wq->buf().size();) {
    auto const chunk_sz{
        std::min(cache_->item_sz() - chunk_offset, wq->buf().size() - wb),
    };

    auto chunk_wq{
        wq->subquery(wb, chunk_sz, wq->offset() + wb,
                     [this, chunk_id, wq](ublk::write_query const &chunk_wq) {
                       cache_->invalidate(chunk_id);
                       if (chunk_wq.err()) [[unlikely]] {
                         wq->set_err(chunk_wq.err());
                         return;
                       }
                     }),
    };

    if (auto const res{handler_->submit(std::move(chunk_wq))}) [[unlikely]] {
      cache_->invalidate(chunk_id);
      return res;
    }

    ++chunk_id;
    chunk_offset = 0;
    wb += chunk_sz;
  }

  return 0;
}

class CachedRWTHandler : public CachedRWHandlerBase {
public:
  explicit CachedRWTHandler(
      std::shared_ptr<ublk::flat_lru_cache<uint64_t, std::byte>> cache,
      std::shared_ptr<ublk::IRWHandler> handler,
      std::shared_ptr<mem_chunk_pool> mem_chunk_pool) noexcept;
  ~CachedRWTHandler() override = default;

  CachedRWTHandler(CachedRWTHandler const &) = delete;
  CachedRWTHandler &operator=(CachedRWTHandler const &) = delete;

  CachedRWTHandler(CachedRWTHandler &&) = delete;
  CachedRWTHandler &operator=(CachedRWTHandler &&) = delete;

  int submit(std::shared_ptr<ublk::write_query> wq) noexcept override;

private:
  int process(std::shared_ptr<ublk::write_query> wq) noexcept;

  boost::dynamic_bitset<uint64_t> chunk_write_lock_state_;
  std::vector<std::pair<uint64_t, std::shared_ptr<ublk::write_query>>>
      wqs_pending_;
};

CachedRWTHandler::CachedRWTHandler(
    std::shared_ptr<ublk::flat_lru_cache<uint64_t, std::byte>> cache,
    std::shared_ptr<ublk::IRWHandler> handler,
    std::shared_ptr<mem_chunk_pool> mem_chunk_pool) noexcept
    : CachedRWHandlerBase(std::move(cache), std::move(handler),
                          std::move(mem_chunk_pool)) {}

int CachedRWTHandler::process(std::shared_ptr<ublk::write_query> wq) noexcept {
  assert(wq);

  auto chunk_id{wq->offset() / cache_->item_sz()};
  auto chunk_offset{wq->offset() % cache_->item_sz()};

  if (auto const chunk{wq->buf()}; !(chunk.size() < cache_->item_sz())) {
    assert(chunk.size() == cache_->item_sz());
    assert(0 == chunk_offset);

    auto mem_chunk = mem_chunk_pool_->get();
    assert(mem_chunk);

    ublk::algo::copy(chunk, mem_chunk_view(mem_chunk));

    cache_full_line_update(chunk_id, std::move(mem_chunk));
  } else if (auto cached_chunk = cache_->find_mutable(chunk_id);
             !cached_chunk.empty()) {
    assert(!(chunk_offset + chunk.size() > cached_chunk.size()));
    auto const from{chunk};
    auto const to{cached_chunk.subspan(chunk_offset, chunk.size())};
    ublk::algo::copy(from, to);
  } else {
    auto mem_chunk = mem_chunk_pool_->get();
    assert(mem_chunk);

    auto mem_chunk_span = mem_chunk_view(mem_chunk);
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

          cache_full_line_update(chunk_id, std::move(*mem_chunk_holder.get()));

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

  if (auto const chunk_id_last{
          (wq->offset() + wq->buf().size()) / cache_->item_sz(),
      };
      !(chunk_id_last < chunk_write_lock_state_.size())) [[unlikely]] {

    chunk_write_lock_state_.resize(chunk_id_last + 1);
  }

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
                  chunk_write_lock_state_.reset(chunk_id);
                  finish = true;
                }
              }
            }),
    };

    if (chunk_write_lock_state_.test_set(chunk_id)) [[unlikely]] {
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
  auto pool = std::make_shared<mem_chunk_pool>(mm::get_unique_bytes_generator(
      kCachedChunkAlignment, cache_sp->item_sz()));

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
