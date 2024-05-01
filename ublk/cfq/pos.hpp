#pragma once

#include <concepts>
#include <functional>
#include <memory>

namespace ublk::cfq {
template <std::integral T>
using pos_t = std::unique_ptr<T, std::function<void(T *)>>;
} // namespace ublk::cfq
