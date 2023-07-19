#pragma once

#include <cstddef>

#include <bits/types.h>
#include <sys/types.h>

#include "handler_interface.hpp"

namespace cfq {
using IDiscardHandler = IHandler<ssize_t(__off64_t, size_t) noexcept>;
} // namespace cfq
