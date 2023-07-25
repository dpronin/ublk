#pragma once

#include <type_traits>

namespace ublk {
template <typename T>
concept cfq_suitable = std::is_trivial_v<T> && std::is_trivially_copyable_v<T>;
}
