#pragma once

#include <cstddef>
#include <cstdint>

#include <memory>

#include "mm/cache_line_aligned_allocator.hpp"
#include "mm/mem_chunk_pool.hpp"

#include "utils/bitset_locker.hpp"

#include "rw_handler_interface.hpp"

#include "flat_lru.hpp"
#include "write_query.hpp"

namespace ublk::cache {

class RWIHandler : public IRWHandler {
public:
  explicit RWIHandler(
      std::shared_ptr<cache::flat_lru<uint64_t, std::byte>> cache,
      std::shared_ptr<IRWHandler> handler,
      std::shared_ptr<mm::mem_chunk_pool> mem_chunk_pool) noexcept;
  ~RWIHandler() override = default;

  RWIHandler(RWIHandler const &) = delete;
  RWIHandler &operator=(RWIHandler const &) = delete;

  RWIHandler(RWIHandler &&) = delete;
  RWIHandler &operator=(RWIHandler &&) = delete;

  int submit(std::shared_ptr<read_query> rq) noexcept override;
  int submit(std::shared_ptr<write_query> wq) noexcept override;

protected:
  std::shared_ptr<cache::flat_lru<uint64_t, std::byte>> cache_;
  std::shared_ptr<IRWHandler> handler_;
  std::shared_ptr<mm::mem_chunk_pool> mem_chunk_pool_;
  uint64_t last_wq_done_seq_;
};

} // namespace ublk::cache
