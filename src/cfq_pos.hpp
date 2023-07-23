#pragma once

#include <concepts>

#include "mapping.hpp"

namespace ublk {
template <std::integral T> using pos_t = uptrwd<T>;
} // namespace ublk
