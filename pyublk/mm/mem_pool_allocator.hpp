#pragma once

#include <cstddef>
#include <cstdlib>

#include <memory>
#include <stack>
#include <utility>

#include "mem_allocator.hpp"
#include "utility.hpp"

namespace ublk::mm {

template <typename T, size_t Alignment = alignof(T)> class mem_pool_allocator {
public:
  static_assert(is_power_of_2(Alignment), "Alignment must be a power of 2");
  static_assert(alignof(T) <= Alignment,
                "Alignment must be at least alignof(T)");

  explicit mem_pool_allocator(std::shared_ptr<mem_allocator> a = {}) noexcept
      : a_(std::move(a)) {
    if (!a_)
      a_ = std::make_shared<mem_allocator>();
  }
  virtual ~mem_pool_allocator() = default;

  mem_pool_allocator(mem_pool_allocator const &) = delete;
  mem_pool_allocator &operator=(mem_pool_allocator const &) = delete;

  mem_pool_allocator(mem_pool_allocator &&) = delete;
  mem_pool_allocator &operator=(mem_pool_allocator &&) = delete;

  virtual void *allocate() noexcept {
    if (!free_.empty()) [[likely]] {
      auto *p = free_.top();
      free_.pop();
      return p;
    }
    return a_->allocate_aligned(Alignment, sizeof(T));
  }

  virtual void free(void *p) noexcept { free_.push(p); }

private:
  std::shared_ptr<mem_allocator> a_;
  std::stack<void *> free_;
};

} // namespace ublk::mm
