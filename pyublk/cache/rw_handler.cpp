#include "rw_handler.hpp"

#include <cassert>

#include <memory>
#include <utility>

#include "mm/mem.hpp"
#include "mm/mem_chunk_pool.hpp"

#include "utils/utility.hpp"

#include "read_query.hpp"
#include "rwi_handler.hpp"
#include "rwt_handler.hpp"
#include "sector.hpp"
#include "write_query.hpp"

namespace ublk::cache {

RWHandler::RWHandler(std::unique_ptr<flat_lru<uint64_t, std::byte>> cache,
                     std::unique_ptr<IRWHandler> handler,
                     bool write_through /* = true*/) {
  assert(!(cache->item_sz() < kSectorSz));
  assert(is_power_of_2(cache->item_sz()));

  auto cache_sp{std::shared_ptr{std::move(cache)}};
  auto handler_sp{std::shared_ptr{std::move(handler)}};
  auto pool = std::make_shared<mm::mem_chunk_pool>(
      mm::get_unique_bytes_generator(kCachedChunkAlignment,
                                     cache_sp->item_sz()),
      kCachedChunkAlignment, cache_sp->item_sz());

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
