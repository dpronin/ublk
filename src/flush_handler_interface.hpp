#pragma once

#include "handler_interface.hpp"

namespace cfq {
using IFlushHandler = IHandler<int() noexcept>;
} // namespace cfq
