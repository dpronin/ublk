#pragma once

#include <concepts>

#include "mapping.hpp"

namespace cfq {
template <std::integral T> using pos_t = cfq::uptrwd<T>;
} // namespace cfq
