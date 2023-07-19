#pragma once

#include <functional>
#include <memory>
#include <new>

#include "align.hpp"

namespace cfq {

template <typename T> using memd_t = std::function<void(T *p)>;
template <typename T> using mem_t = std::unique_ptr<T, memd_t<T>>;
template <typename T> using uptrwd = mem_t<T>;

template <typename T, typename... Args>
uptrwd<T> make_unique_aligned(size_t align, Args &&...args) {
  return uptrwd<T>{new (std::align_val_t{align}) T{std::forward<Args>(args)...},
                   [align](T *p) {
                     std::destroy_at(p);
                     operator delete(p, std::align_val_t{align});
                   }};
}

} // namespace cfq
