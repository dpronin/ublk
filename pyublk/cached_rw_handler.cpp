#include "cached_rw_handler.hpp"

#include <cassert>
#include <cstdint>

#include <utility>

#include "algo.hpp"

namespace ublk {

CachedRWHandler::CachedRWHandler(
    std::unique_ptr<flat_lru_cache<uint64_t, std::byte>> cache,
    std::unique_ptr<IRWHandler> handler, bool write_through /* = true*/)
    : cache_(std::move(cache)), handler_(std::move(handler)) {
  assert(cache_);
  assert(handler_);
  if (write_through) {
    cache_updater_ = [this](uint64_t chunk_id_at, uint64_t chunk_offset_at,
                            std::span<std::byte const> chunk) {
      /* update cache with a corresponding data from the chunk */
      auto [cached_chunk, _]{cache_->find_allocate_mutable(chunk_id_at)};
      assert(!cached_chunk.empty());

      auto const from{chunk};
      auto const to{cached_chunk.subspan(chunk_offset_at, chunk.size())};

      algo::copy(from, to);
    };
  } else {
    cache_updater_ = [this](uint64_t chunk_id_at,
                            uint64_t chunk_offset_at [[maybe_unused]],
                            std::span<std::byte const> chunk [[maybe_unused]]) {
      cache_->invalidate(chunk_id_at);
    };
  }
}

ssize_t CachedRWHandler::read(std::span<std::byte> buf,
                              __off64_t offset) noexcept {
  ssize_t rb{0};

  auto chunk_id{offset / cache_->item_sz()};
  auto chunk_offset{offset % cache_->item_sz()};

  while (!buf.empty()) {
    auto [cached_chunk, valid] = cache_->find_allocate_mutable(chunk_id);
    assert(!cached_chunk.empty());
    if (valid) {
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
    } else if (auto const res =
                   handler_->read(cached_chunk, chunk_id * cache_->item_sz());
               res < 0) [[unlikely]] {
      cache_->invalidate(chunk_id);
      return res;
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

    cache_updater_(chunk_id, chunk_offset, chunk);

    wb += chunk.size();
    chunk_offset = 0;
    ++chunk_id;
    buf = buf.subspan(chunk.size());
  }

  return wb;
}

} // namespace ublk
