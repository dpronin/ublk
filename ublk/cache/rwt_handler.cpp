#include "rwt_handler.hpp"

#include <cstddef>
#include <cstdint>

#include <utility>

#include <gsl/assert>

#include "utils/algo.hpp"

namespace ublk::cache {

RWTHandler::RWTHandler(
    std::shared_ptr<cache::flat_lru<uint64_t, std::byte>> cache,
    std::shared_ptr<IRWHandler> handler,
    std::shared_ptr<mm::mem_chunk_pool> mem_chunk_pool) noexcept
    : RWIHandler(std::move(cache), std::move(handler),
                 std::move(mem_chunk_pool)) {}

int RWTHandler::process(std::shared_ptr<write_query> wq) noexcept {
  Expects(wq);
  Expects(!wq->buf().empty());

  auto chunk_id{wq->offset() / cache_->item_sz()};
  auto chunk_offset{wq->offset() % cache_->item_sz()};

  if (auto const chunk{wq->buf()}; !(chunk.size() < cache_->item_sz())) {
    Ensures(chunk.size() == cache_->item_sz());
    Ensures(0 == chunk_offset);

    auto mem_chunk = mem_chunk_pool_->get();
    Ensures(mem_chunk);

    algo::copy(chunk, mem_chunk_pool_->chunk_view(mem_chunk));

    cache_->update({chunk_id, std::move(mem_chunk)});
    ++last_wq_done_seq_;
  } else if (auto cached_chunk = cache_->find_mutable(chunk_id);
             !cached_chunk.empty()) {
    Expects(!(chunk_offset + chunk.size() > cached_chunk.size()));
    auto const from{chunk};
    auto const to{cached_chunk.subspan(chunk_offset, chunk.size())};
    algo::copy(from, to);
  } else {
    auto mem_chunk = mem_chunk_pool_->get();
    Ensures(mem_chunk);

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

          if (auto const res [[maybe_unused]]{handler_->submit(std::move(wq))})
              [[unlikely]] {
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
  Expects(wq);
  Expects(!wq->buf().empty());

  size_t wb{0};

  auto chunk_id{wq->offset() / cache_->item_sz()};

  if (auto const chunk_off{wq->offset() % cache_->item_sz()}; chunk_off > 0) {
    auto const chunk_sz{
        std::min(cache_->item_sz() - chunk_off, wq->buf().size()),
    };

    auto const chunk{wq->buf().subspan(0, chunk_sz)};

    if (auto cached_chunk = cache_->find_mutable(chunk_id);
        !cached_chunk.empty()) {
      assert(!(chunk_off + chunk.size() > cached_chunk.size()));
      auto const from{chunk};
      auto const to{cached_chunk.subspan(chunk_off, chunk.size())};
      algo::copy(from, to);
    } else {
      auto mem_chunk = mem_chunk_pool_->get();
      assert(mem_chunk);

      auto mem_chunk_span = mem_chunk_pool_->chunk_view(mem_chunk);
      auto rmwq = read_query::create(
          mem_chunk_span, chunk_id * cache_->item_sz(),
          [this, wq = wq->subquery(0, chunk.size(), wq->offset(), wq), chunk_id,
           mem_chunk_holder = std::make_shared<decltype(mem_chunk)>(
               std::move(mem_chunk))](read_query const &rmwq) mutable {
            if (rmwq.err()) [[unlikely]] {
              wq->set_err(rmwq.err());
              return;
            }

            cache_->update({chunk_id, std::move(*mem_chunk_holder.get())});
            ++last_wq_done_seq_;

            if (auto const r{submit(wq)}) {
              wq->set_err(r);
              return;
            }
          });
      if (auto const r{handler_->submit(std::move(rmwq))}) [[unlikely]] {
        return r;
      }
    }

    ++chunk_id;
    wb += chunk.size();
  }

  for (; wb < wq->buf().size(); ++chunk_id) {
    auto const chunk_sz{
        std::min(cache_->item_sz(), wq->buf().size() - wb),
    };

    auto chunk_wq_completer{
        [this, chunk_id, wq](write_query const &chunk_wq) {
          auto wq_it{wqs_pending_.find(chunk_id)};
          assert(wqs_pending_.end() != wq_it);

          bool submitted{false};
          while (!wq_it->second.empty() && !submitted) {
            submitted = 0 == process(wq_it->second.front());
            wq_it->second.pop();
          }

          if (!submitted)
            wqs_pending_.erase(wq_it);

          if (chunk_wq.err()) [[unlikely]] {
            cache_->invalidate(chunk_id);
            wq->set_err(chunk_wq.err());
          }
        },
    };

    auto chunk_wq{
        wq->subquery(wb, chunk_sz, wq->offset() + wb,
                     std::move(chunk_wq_completer)),
    };

    if (auto wq_it = wqs_pending_.find(chunk_id); wqs_pending_.end() != wq_it) {
      wq_it->second.push(std::move(chunk_wq));
    } else if (auto const res{process(std::move(chunk_wq))}) [[unlikely]] {
      wqs_pending_.emplace_hint(wq_it, chunk_id,
                                std::queue<std::shared_ptr<write_query>>{});
      return res;
    }

    wb += chunk_sz;
  }

  return 0;
}

} // namespace ublk::cache
