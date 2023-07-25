#pragma once

#include <cstddef>

#include <bits/types.h>
#include <sys/types.h>

#include <span>

#include "handler_interface.hpp"

namespace ublk {
using IWriteHandler =
    IHandler<ssize_t(std::span<std::byte const>, __off64_t) noexcept>;
} // namespace ublk
