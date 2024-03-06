#pragma once

#include <cstddef>
#include <cstdint>

#include <memory>
#include <utility>
#include <vector>

#include "mm/cache_line_aligned_allocator.hpp"
#include "mm/mem_chunk_pool.hpp"

#include "utils/bitset_locker.hpp"

#include "write_query.hpp"

#include "flat_lru.hpp"

#include "rwi_handler.hpp"

namespace ublk::cache {

class RWTHandler : public RWIHandler {
public:
  explicit RWTHandler(
      std::shared_ptr<cache::flat_lru<uint64_t, std::byte>> cache,
      std::shared_ptr<IRWHandler> handler,
      std::shared_ptr<mm::mem_chunk_pool> mem_chunk_pool) noexcept;
  ~RWTHandler() override = default;

  RWTHandler(RWTHandler const &) = delete;
  RWTHandler &operator=(RWTHandler const &) = delete;

  RWTHandler(RWTHandler &&) = delete;
  RWTHandler &operator=(RWTHandler &&) = delete;

  int submit(std::shared_ptr<write_query> wq) noexcept override;

private:
  int process(std::shared_ptr<write_query> wq) noexcept;

  bitset_locker<uint64_t, mm::allocator::cache_line_aligned_allocator<uint64_t>>
      chunk_w_locker_;
  std::vector<std::pair<uint64_t, std::shared_ptr<write_query>>> wqs_pending_;
};

} // namespace ublk::cache
