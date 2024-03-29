#pragma once

#include "utils/align.hpp"

#include "pool_allocator.hpp"

namespace ublk::mm::allocator {

template <typename T> struct pool_cache_line_aligned {
  static inline auto value =
      pool_allocator<T, hardware_destructive_interference_size>{};
};

} // namespace ublk::mm::allocator
