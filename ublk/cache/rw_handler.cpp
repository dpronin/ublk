#include "rw_handler.hpp"

#include <cassert>

#include <memory>
#include <utility>

#include "mm/mem.hpp"
#include "mm/mem_chunk_pool.hpp"

#include "utils/size_units.hpp"
#include "utils/utility.hpp"

#include "flat_lru.hpp"
#include "read_query.hpp"
#include "rwi_handler.hpp"
#include "rwt_handler.hpp"
#include "sector.hpp"
#include "write_query.hpp"

namespace {

std::unique_ptr<ublk::cache::flat_lru<uint64_t, std::byte>>
make_cache(uint64_t cache_len_bytes) {
  auto cache = std::unique_ptr<ublk::cache::flat_lru<uint64_t, std::byte>>{};
  if (cache_len_bytes) {
    using namespace ublk::literals;
    for (uint64_t cache_item_sz = 1_MiB;
         !cache && !(cache_item_sz < ublk::kSectorSz); cache_item_sz >>= 1) {
      if (auto const cache_len = cache_len_bytes / cache_item_sz) {
        cache = ublk::cache::flat_lru<uint64_t, std::byte>::create(
            ublk::div_round_up(cache_len_bytes, cache_item_sz), cache_item_sz);
      }
    }
  }
  return cache;
}

} // namespace

namespace ublk::cache {

RWHandler::RWHandler(uint64_t cache_len_sectors,
                     std::unique_ptr<IRWHandler> handler,
                     bool write_through /* = true*/) {
  auto cache_sp{
      std::shared_ptr{make_cache(sectors_to_bytes(cache_len_sectors))},
  };
  assert(cache_sp);

  auto handler_sp{std::shared_ptr{std::move(handler)}};

  auto pool = std::make_shared<mm::mem_chunk_pool>(kCachedChunkAlignment,
                                                   cache_sp->item_sz());

  handlers_[0] = std::make_shared<RWIHandler>(cache_sp, handler_sp, pool);
  handlers_[1] = std::make_shared<RWTHandler>(cache_sp, handler_sp, pool);

  set_write_through(write_through);
}

void RWHandler::set_write_through(bool value) noexcept {
  handler_ = handlers_[!!value];
}

bool RWHandler::write_through() const noexcept {
  return handler_ == handlers_[1];
}

int RWHandler::submit(std::shared_ptr<read_query> rq) noexcept {
  return handler_->submit(std::move(rq));
}

int RWHandler::submit(std::shared_ptr<write_query> wq) noexcept {
  return handler_->submit(std::move(wq));
}

} // namespace ublk::cache
