#pragma once

#include <concepts>

#include "mm/mem.hpp"

namespace ublk {
template <std::integral T> using pos_t = mm::uptrwd<T>;
} // namespace ublk
