#pragma once

#include "handler_interface.hpp"

namespace ublk {
using IFlushHandler = IHandler<int() noexcept>;
} // namespace ublk
