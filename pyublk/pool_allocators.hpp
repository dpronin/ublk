#pragma once

#include "align.hpp"
#include "pool_allocator.hpp"

namespace ublk::mem::allocator {

template <typename T> struct pool_cache_line_aligned {
  static inline auto value =
      pool_allocator<T, hardware_destructive_interference_size>{};
};

} // namespace ublk::mem::allocator
