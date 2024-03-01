#pragma once

#include "align.hpp"
#include "aligned_allocator.hpp"

namespace ublk::mm {
template <typename T>
using cache_line_aligned_allocator =
    class aligned_allocator<T, hardware_destructive_interference_size>;
} // namespace ublk::mm
