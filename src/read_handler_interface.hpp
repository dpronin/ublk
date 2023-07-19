#pragma once

#include <cstddef>

#include <bits/types.h>
#include <sys/types.h>

#include <span>

#include "handler_interface.hpp"

namespace cfq {
using IReadHandler =
    IHandler<ssize_t(std::span<std::byte>, __off64_t) noexcept>;
} // namespace cfq
