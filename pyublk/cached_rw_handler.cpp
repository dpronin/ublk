#include "cached_rw_handler.hpp"

#include <cassert>
#include <cstdint>

#include <utility>

#include "algo.hpp"
#include "mem.hpp"
#include "read_query.hpp"
#include "write_query.hpp"

namespace ublk {

CachedRWHandler::CachedRWHandler(
    std::unique_ptr<flat_lru_cache<uint64_t, std::byte>> cache,
    std::unique_ptr<IRWHandler> handler, bool write_through /* = true*/)
    : cache_(std::move(cache)), handler_(std::move(handler)) {
  assert(cache_);
  assert(handler_);
  set_write_through(write_through);
  cached_chunk_generator_ =
      get_unique_bytes_generator(kCachedChunkAlignment, cache_->item_sz());
}

void CachedRWHandler::set_write_through(bool value) noexcept {
  if (value) {
    cache_updater_ = [this](uint64_t chunk_id,
                            std::span<std::byte const> chunk) {
      if (chunk.size() < cache_->item_sz()) {
        cache_->invalidate(chunk_id);
      } else {
        auto cached_chunk = cached_chunk_generator_();

        auto const from{chunk};
        auto const to{cached_chunk_view(cached_chunk)};

        algo::copy(from, to);

        auto evicted_value{
            cache_->update({chunk_id, std::move(cached_chunk)}),
        };
        assert(evicted_value);
      }
    };
  } else {
    cache_updater_ = [this](uint64_t chunk_id, std::span<std::byte const> chunk
                            [[maybe_unused]]) { cache_->invalidate(chunk_id); };
  }
  write_through_ = value;
}

bool CachedRWHandler::write_through() const noexcept { return write_through_; }

int CachedRWHandler::submit(std::shared_ptr<read_query> rq) noexcept {
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
      algo::copy(std::as_bytes(from), to);
    } else {
      auto new_cached_chunk = cached_chunk_generator_();
      auto new_cached_chunk_span = cached_chunk_view(new_cached_chunk);
      auto new_rq = read_query::create(
          new_cached_chunk_span, chunk_id * cache_->item_sz(),
          [this, chunk_id, chunk_offset, chunk, rq,
           cached_chunk = std::make_shared<decltype(new_cached_chunk)>(
               std::move(new_cached_chunk))](read_query const &new_rq) mutable {
            if (new_rq.err()) {
              rq->set_err(new_rq.err());
              cache_->invalidate(chunk_id);
              return;
            }

            auto const from{new_rq.buf().subspan(chunk_offset, chunk.size())};
            auto const to{chunk};
            algo::copy(std::as_bytes(from), to);

            auto evicted_value{
                cache_->update({chunk_id, std::move(*cached_chunk.get())})};
            assert(evicted_value);
          });
      if (auto const res{handler_->submit(std::move(new_rq))}) [[unlikely]] {
        cache_->invalidate(chunk_id);
        return res;
      }
    }

    ++chunk_id;
    chunk_offset = 0;
    rb += chunk.size();
  }

  return 0;
}

int CachedRWHandler::submit(std::shared_ptr<write_query> wq) noexcept {
  assert(wq);

  auto chunk_id{wq->offset() / cache_->item_sz()};
  auto chunk_offset{wq->offset() % cache_->item_sz()};

  for (size_t wb{0}; wb < wq->buf().size();) {
    auto const chunk_sz{
        std::min(cache_->item_sz() - chunk_offset, wq->buf().size() - wb),
    };
    auto chunk_wq{
        wq->subquery(wb, chunk_sz, wq->offset() + wb,
                     [this, chunk_id, wq](write_query const &chunk_wq) {
                       if (chunk_wq.err()) [[unlikely]] {
                         cache_->invalidate(chunk_id);
                         wq->set_err(chunk_wq.err());
                         return;
                       }
                       cache_updater_(chunk_id, chunk_wq.buf());
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

} // namespace ublk
