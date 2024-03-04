#pragma once

#include "aligned_allocator.hpp"

#include "utils/align.hpp"

namespace ublk::mm {
template <typename T>
using cache_line_aligned_allocator =
    class aligned_allocator<T, hardware_destructive_interference_size>;
} // namespace ublk::mm
