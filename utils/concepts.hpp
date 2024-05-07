#pragma once

#include <type_traits>

namespace ublk {

template <typename T>
concept cfq_suitable = std::is_standard_layout_v<T>;

template <typename T>
concept standard_layout = std::is_standard_layout_v<T>;

template <typename T>
concept is_byte = std::is_same_v<std::remove_const_t<T>, std::byte>;

template <typename T>
concept is_trivial = std::is_trivial_v<T>;

template <typename T>
concept is_trivially_copyable = std::is_trivially_copyable_v<T>;

} // namespace ublk
