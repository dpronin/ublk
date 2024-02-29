#pragma once

#include <memory>

#include "cache_line_aligned_allocator.hpp"
#include "mem_cached_allocator.hpp"

namespace ublk::mem::allocator {

inline auto __mem_cached_allocator__ = std::make_shared<mem_cached_allocator>();

template <typename T> struct cache_line_aligned {
  static inline auto value =
      cache_line_aligned_allocator<T>{__mem_cached_allocator__};
};

} // namespace ublk::mem::allocator
