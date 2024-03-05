#pragma once

#include "aligned_allocator.hpp"

#include "utils/align.hpp"

namespace ublk::mm::allocator {
template <typename T>
using cache_line_aligned_allocator =
    aligned_allocator<T, hardware_destructive_interference_size>;
} // namespace ublk::mm::allocator
