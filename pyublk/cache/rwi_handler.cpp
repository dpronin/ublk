#include "rwi_handler.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>

#include <algorithm>
#include <memory>
#include <utility>

#include "mm/generic_allocators.hpp"

#include "utils/algo.hpp"
#include "utils/utility.hpp"

namespace ublk::cache {

RWIHandler::RWIHandler(
    std::shared_ptr<cache::flat_lru<uint64_t, std::byte>> cache,
    std::shared_ptr<IRWHandler> handler,
    std::shared_ptr<mm::mem_chunk_pool> mem_chunk_pool) noexcept
    : cache_(std::move(cache)), handler_(std::move(handler)),
      mem_chunk_pool_(std::move(mem_chunk_pool)),
      chunk_rw_semaphore_(0,
                          mm::allocator::cache_line_aligned<uint64_t>::value) {
  assert(cache_);
  assert(handler_);
  assert(mem_chunk_pool_);
}

int RWIHandler::submit(std::shared_ptr<read_query> rq) noexcept {
  assert(rq);
  assert(0 != rq->buf().size());

  chunk_rw_semaphore_.extend(
      div_round_up(rq->offset() + rq->buf().size(), cache_->item_sz()));

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
      algo::copy(from, to);
    } else if (chunk_rw_semaphore_.try_read_lock(chunk_id)) [[likely]] {
      auto mem_chunk = mem_chunk_pool_->get();
      assert(mem_chunk);

      auto mem_chunk_span = mem_chunk_pool_->chunk_view(mem_chunk);

      auto chunk_rq = read_query::create(
          mem_chunk_span, chunk_id * cache_->item_sz(),
          [=, this,
           mem_chunk_holder = std::make_shared<decltype(mem_chunk)>(
               std::move(mem_chunk))](read_query const &new_rq) mutable {
            if (new_rq.err()) [[unlikely]] {
              rq->set_err(new_rq.err());
              return;
            }

            auto const from{new_rq.buf().subspan(chunk_offset, chunk.size())};
            auto const to{chunk};
            algo::copy(from, to);

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
              algo::copy(from, to);
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

int RWIHandler::submit(std::shared_ptr<write_query> wq) noexcept {
  assert(wq);
  assert(0 != wq->buf().size());

  auto const chunk_id_first{wq->offset() / cache_->item_sz()};
  auto const chunk_id_last{
      div_round_up(wq->offset() + wq->buf().size(), cache_->item_sz()) - 1,
  };

  auto new_wq{
      wq->subquery(0, wq->buf().size(), wq->offset(),
                   [=, this](write_query const &chunk_wq) {
                     cache_->invalidate_range({chunk_id_first, chunk_id_last});
                     if (chunk_wq.err()) [[unlikely]] {
                       wq->set_err(chunk_wq.err());
                       return;
                     }
                   }),
  };

  if (auto const res{handler_->submit(std::move(new_wq))}) [[unlikely]] {
    return res;
  }

  return 0;
}

} // namespace ublk::cache
