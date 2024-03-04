#pragma once

#include <cstddef>
#include <cstdlib>

namespace ublk::mm {

class mem_allocator {
public:
  mem_allocator() = default;
  virtual ~mem_allocator() = default;

  mem_allocator(mem_allocator const &) = delete;
  mem_allocator &operator=(mem_allocator const &) = delete;

  mem_allocator(mem_allocator &&) = delete;
  mem_allocator &operator=(mem_allocator &&) = delete;

  virtual void *allocate_aligned(size_t alignment, size_t sz) noexcept {
    return std::aligned_alloc(alignment, sz);
  }

  virtual void *allocate(size_t sz) noexcept { return std::malloc(sz); }

  virtual void free(void *p) noexcept { return std::free(p); }
};

} // namespace ublk::mm
