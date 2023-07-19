#pragma once

#include <concepts>

namespace cfq {
template <typename T>
concept cfq_suitable = std::is_trivial_v<T> && std::is_trivially_copyable_v<T>;
}
