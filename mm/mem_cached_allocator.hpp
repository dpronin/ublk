#pragma once

#include <cstddef>
#include <cstdint>

#include <map>
#include <stack>
#include <unordered_map>

#include "mem_allocator.hpp"
#include "mem_types.hpp"

namespace ublk::mm {

class mem_cached_allocator : public mem_allocator {
private:
  struct free_desc {
    std::stack<uptrwd<std::byte[]>> free_chunks;
  };
  using free_desc_table_t = std::map<size_t, free_desc>;

  struct busy_desc {
    uptrwd<std::byte[]> chunk;
    free_desc_table_t::iterator ftbl_desc_it;
  };
  using free_table_t = std::map<size_t, free_desc_table_t>;
  using busy_table_t = std::unordered_map<uintptr_t, busy_desc>;

public:
  mem_cached_allocator() = default;
  ~mem_cached_allocator() override = default;

  mem_cached_allocator(mem_cached_allocator const &) = delete;
  mem_cached_allocator &operator=(mem_cached_allocator const &) = delete;

  mem_cached_allocator(mem_cached_allocator &&) = delete;
  mem_cached_allocator &operator=(mem_cached_allocator &&) = delete;

  void *allocate_aligned(size_t alignment, size_t sz) noexcept override;

  void *allocate(size_t sz) noexcept override {
    return allocate_aligned(alignof(std::max_align_t), sz);
  }

  void free(void *p [[maybe_unused]]) noexcept override;

private:
  free_table_t ftbl_;
  busy_table_t btbl_;
};

inline auto __mem_cached_allocator__ = std::make_shared<mem_cached_allocator>();

} // namespace ublk::mm
