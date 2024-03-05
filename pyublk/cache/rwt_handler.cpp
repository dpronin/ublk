#include "rwt_handler.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>

#include <utility>

#include "utils/algo.hpp"

namespace ublk::cache {

int RWTHandler::process(std::shared_ptr<write_query> wq) noexcept {
  assert(wq);

  auto chunk_id{wq->offset() / cache_->item_sz()};
  auto chunk_offset{wq->offset() % cache_->item_sz()};

  if (auto const chunk{wq->buf()}; !(chunk.size() < cache_->item_sz())) {
    assert(chunk.size() == cache_->item_sz());
    assert(0 == chunk_offset);

    auto mem_chunk = mem_chunk_pool_->get();
    assert(mem_chunk);

    algo::copy(chunk, mem_chunk_pool_->chunk_view(mem_chunk));

    cache_->update({chunk_id, std::move(mem_chunk)});
  } else if (auto cached_chunk = cache_->find_mutable(chunk_id);
             !cached_chunk.empty()) {
    assert(!(chunk_offset + chunk.size() > cached_chunk.size()));
    auto const from{chunk};
    auto const to{cached_chunk.subspan(chunk_offset, chunk.size())};
    algo::copy(from, to);
  } else {
    auto mem_chunk = mem_chunk_pool_->get();
    assert(mem_chunk);

    auto mem_chunk_span = mem_chunk_pool_->chunk_view(mem_chunk);
    auto rmwq = read_query::create(
        mem_chunk_span, chunk_id * cache_->item_sz(),
        [this, chunk_id, chunk_offset, chunk, mem_chunk_span,
         wq = std::move(wq),
         mem_chunk_holder = std::make_shared<decltype(mem_chunk)>(
             std::move(mem_chunk))](read_query const &rmwq) mutable {
          if (rmwq.err()) [[unlikely]] {
            wq->set_err(rmwq.err());
            return;
          }

          auto const from{chunk};
          auto const to{mem_chunk_span.subspan(chunk_offset, chunk.size())};
          algo::copy(from, to);

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

int RWTHandler::submit(std::shared_ptr<write_query> wq) noexcept {
  assert(wq);
  assert(0 != wq->buf().size());

  chunk_rw_semaphore_.extend(
      div_round_up(wq->offset() + wq->buf().size(), cache_->item_sz()));

  auto chunk_id{wq->offset() / cache_->item_sz()};
  auto chunk_offset{wq->offset() % cache_->item_sz()};

  for (size_t wb{0}; wb < wq->buf().size();) {
    auto const chunk_sz{
        std::min(cache_->item_sz() - chunk_offset, wq->buf().size() - wb),
    };

    auto chunk_wq{
        wq->subquery(
            wb, chunk_sz, wq->offset() + wb,
            [this, chunk_id, wq](write_query const &chunk_wq) {
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

} // namespace ublk::cache
