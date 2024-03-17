#include "rwt_handler.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>

#include <utility>

#include "mm/generic_allocators.hpp"

#include "utils/algo.hpp"

namespace ublk::cache {

RWTHandler::RWTHandler(
    std::shared_ptr<cache::flat_lru<uint64_t, std::byte>> cache,
    std::shared_ptr<IRWHandler> handler,
    std::shared_ptr<mm::mem_chunk_pool> mem_chunk_pool) noexcept
    : RWIHandler(std::move(cache), std::move(handler),
                 std::move(mem_chunk_pool)),
      chunk_w_locker_(0, mm::allocator::cache_line_aligned<uint64_t>::value) {}

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
    ++last_wq_done_seq_;
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
          ++last_wq_done_seq_;

          if (auto const res{handler_->submit(std::move(wq))}) [[unlikely]] {
            return;
          }
        });
    if (auto const res{handler_->submit(std::move(rmwq))}) [[unlikely]] {
      return res;
    }
  }

  if (wq) {
    if (auto const res{handler_->submit(std::move(wq))}) [[unlikely]] {
      return res;
    }
  }

  return 0;
}

int RWTHandler::submit(std::shared_ptr<write_query> wq) noexcept {
  assert(wq);
  assert(0 != wq->buf().size());

  chunk_w_locker_.extend(
      div_round_up(wq->offset() + wq->buf().size(), cache_->item_sz()));

  auto chunk_id{wq->offset() / cache_->item_sz()};
  auto chunk_offset{wq->offset() % cache_->item_sz()};

  for (size_t wb{0}; wb < wq->buf().size(); ++chunk_id, chunk_offset = 0) {
    auto const chunk_sz{
        std::min(cache_->item_sz() - chunk_offset, wq->buf().size() - wb),
    };

    auto chunk_wq_completer{
        [this, chunk_id, wq](write_query const &chunk_wq) {
          for (bool finish = false; !finish;) {
            if (auto next_wq_it = std::ranges::find(
                    wqs_pending_, chunk_id,
                    [](auto const &wq_pend) { return wq_pend.first; });
                next_wq_it != wqs_pending_.end()) {
              finish = 0 == process(next_wq_it->second);
              std::iter_swap(next_wq_it, wqs_pending_.end() - 1);
              wqs_pending_.pop_back();
            } else {
              chunk_w_locker_.unlock(chunk_id);
              finish = true;
            }

            if (chunk_wq.err()) [[unlikely]] {
              cache_->invalidate(chunk_id);
              wq->set_err(chunk_wq.err());
              return;
            }
          }
        },
    };

    auto chunk_wq{
        wq->subquery(wb, chunk_sz, wq->offset() + wb,
                     std::move(chunk_wq_completer)),
    };

    if (!chunk_w_locker_.try_lock(chunk_id)) [[unlikely]] {
      wqs_pending_.emplace_back(chunk_id, std::move(chunk_wq));
    } else if (auto const res{process(std::move(chunk_wq))}) [[unlikely]] {
      return res;
    }

    wb += chunk_sz;
  }

  return 0;
}

} // namespace ublk::cache
