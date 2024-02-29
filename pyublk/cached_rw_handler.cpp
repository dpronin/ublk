#include "cached_rw_handler.hpp"

#include <cassert>
#include <cstdint>

#include <utility>

#include "algo.hpp"
#include "mem.hpp"

namespace ublk {

CachedRWHandler::CachedRWHandler(
    std::unique_ptr<flat_lru_cache<uint64_t, std::byte>> cache,
    std::unique_ptr<IRWHandler> handler, bool write_through /* = true*/)
    : cache_(std::move(cache)), handler_(std::move(handler)) {
  assert(cache_);
  assert(handler_);
  if (write_through) {
    cache_updater_ = [this](uint64_t chunk_id,
                            std::span<std::byte const> chunk) {
      if (chunk.size() < cache_->item_sz()) {
        cache_->invalidate(chunk_id);
      } else {
        auto const from{chunk};
        auto const to{cached_chunk_view()};

        algo::copy(from, to);

        auto evicted_value{
            cache_->update({chunk_id, std::move(cached_chunk_)}),
        };
        assert(evicted_value);
        cached_chunk_.swap(evicted_value->second);
      }
    };
  } else {
    cache_updater_ = [this](uint64_t chunk_id, std::span<std::byte const> chunk
                            [[maybe_unused]]) { cache_->invalidate(chunk_id); };
  }
  cached_chunk_ =
      get_unique_bytes_generator(kCachedChunkAlignment, cache_->item_sz())();
}

ssize_t CachedRWHandler::read(std::span<std::byte> buf,
                              __off64_t offset) noexcept {
  ssize_t rb{0};

  auto chunk_id{offset / cache_->item_sz()};
  auto chunk_offset{offset % cache_->item_sz()};

  while (!buf.empty()) {
    if (auto cached_chunk = cache_->find(chunk_id); !cached_chunk.empty()) {
      auto const chunk{
          buf.subspan(0,
                      std::min(cache_->item_sz() - chunk_offset, buf.size())),
      };

      auto const from{cached_chunk.subspan(chunk_offset, chunk.size())};
      auto const to{chunk};

      algo::copy(std::as_bytes(from), to);

      ++chunk_id;
      chunk_offset = 0;
      buf = buf.subspan(chunk.size());
      rb += chunk.size();
    } else if (auto const res = handler_->read(cached_chunk_view(),
                                               chunk_id * cache_->item_sz());
               res < 0) [[unlikely]] {
      cache_->invalidate(chunk_id);
      return res;
    } else {
      auto evicted_value{cache_->update({chunk_id, std::move(cached_chunk_)})};
      assert(evicted_value);
      cached_chunk_.swap(evicted_value->second);
    }
  }

  return rb;
}

ssize_t CachedRWHandler::write(std::span<std::byte const> buf,
                               __off64_t offset) noexcept {
  ssize_t wb{0};

  auto chunk_id{offset / cache_->item_sz()};
  auto chunk_offset{offset % cache_->item_sz()};

  while (!buf.empty()) {
    auto const chunk{
        buf.subspan(0, std::min(cache_->item_sz() - chunk_offset, buf.size())),
    };

    if (auto const res{handler_->write(chunk, offset + wb)}; res < 0)
        [[unlikely]] {
      cache_->invalidate(chunk_id);
      return res;
    }

    cache_updater_(chunk_id, chunk);

    ++chunk_id;
    chunk_offset = 0;
    buf = buf.subspan(chunk.size());
    wb += chunk.size();
  }

  return wb;
}

} // namespace ublk
