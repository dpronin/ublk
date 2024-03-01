#pragma once

#include <functional>
#include <memory>
#include <type_traits>

namespace ublk::mm {

template <typename T>
using memd_t = std::function<void(
    std::conditional_t<std::is_unbounded_array_v<T>, T, T *> p)>;
template <typename T> using mem_t = std::unique_ptr<T, memd_t<T>>;
template <typename T> using uptrwd = mem_t<T>;

struct alloc_mode_new {};
struct alloc_mode_mmap {
  int flags{0};
};

} // namespace ublk::mm
