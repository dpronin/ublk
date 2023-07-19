#pragma once

#include "handler_interface.hpp"

namespace cfq {
template <typename T> using ICmdHandler = IHandler<int(T &) noexcept>;
} // namespace cfq
