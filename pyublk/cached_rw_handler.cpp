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
    cache_updater_ = [this](uint64_t chunk_id_at, uint64_t chunk_offset_at,
                            std::span<std::byte const> chunk) {
      auto const from{chunk};
      auto const to{
          std::span{chunk_tmp_.get() + chunk_offset_at, chunk.size()},
      };

      algo::copy(from, to);

      /* update cache with a corresponding data from the chunk */
      auto evicted_value{cache_->update({chunk_id_at, std::move(chunk_tmp_)})};
      assert(evicted_value);
      chunk_tmp_.swap(evicted_value->second);
    };
  } else {
    cache_updater_ = [this](uint64_t chunk_id_at,
                            uint64_t chunk_offset_at [[maybe_unused]],
                            std::span<std::byte const> chunk [[maybe_unused]]) {
      cache_->invalidate(chunk_id_at);
    };
  }
  chunk_tmp_ = get_unique_bytes_generator(cache_->item_sz())();
}

ssize_t CachedRWHandler::read(std::span<std::byte> buf,
                              __off64_t offset) noexcept {
  ssize_t rb{0};

  auto chunk_id{offset / cache_->item_sz()};
  auto chunk_offset{offset % cache_->item_sz()};

  while (!buf.empty()) {
    if (auto cached_chunk = cache_->find_mutable(chunk_id);
        !cached_chunk.empty()) {
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
    } else {
      if (auto const res = handler_->read({chunk_tmp_.get(), cache_->item_sz()},
                                          chunk_id * cache_->item_sz());
          res < 0) [[unlikely]] {
        cache_->invalidate(chunk_id);
        return res;
      }
      auto evicted_value{cache_->update({chunk_id, std::move(chunk_tmp_)})};
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
