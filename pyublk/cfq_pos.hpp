#pragma once

#include <concepts>

#include "mem.hpp"

namespace ublk {
template <std::integral T> using pos_t = uptrwd<T>;
} // namespace ublk
