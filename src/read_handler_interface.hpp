#pragma once

#include <cstddef>

#include <linux/types.h>

#include <span>

#include "handler_interface.hpp"

namespace cfq {
using IReadHandler = IHandler<int(std::span<std::byte>, __off64_t) noexcept>;
} // namespace cfq
