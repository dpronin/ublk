#pragma once

#include <cstddef>

#include <bits/types.h>
#include <sys/types.h>

#include "handler_interface.hpp"

namespace ublk {
using IDiscardHandler = IHandler<ssize_t(__off64_t, size_t) noexcept>;
} // namespace ublk
