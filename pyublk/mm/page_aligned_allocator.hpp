#pragma once

#include <cstddef>
#include <cstdlib>

#include <limits>
#include <new>

#include "page.hpp"

namespace ublk::mm {

template <typename T = void> class page_aligned_allocator {
public:
  template <typename U> struct rebind {
    using other = page_aligned_allocator<U>;
  };

  using value_type = T;

  page_aligned_allocator() = default;
  ~page_aligned_allocator() = default;

  template <typename U>
  constexpr explicit page_aligned_allocator(
      page_aligned_allocator<U> const &other [[maybe_unused]]) noexcept {}

  page_aligned_allocator(page_aligned_allocator<T> const &) = default;
  page_aligned_allocator(page_aligned_allocator<T> &&) = default;

  T *allocate(size_t n) {
    if (n > std::numeric_limits<size_t>::max() / sizeof(T))
      throw std::bad_array_new_length();

    if (auto const alignment = std::max(get_page_size(), alignof(T));
        alignment > 0) {
      if (auto *p =
              static_cast<T *>(std::aligned_alloc(alignment, sizeof(T) * n)))
        return p;
    }

    throw std::bad_alloc();
  }

  void deallocate(T *p, size_t n [[maybe_unused]]) noexcept { std::free(p); }
};

} // namespace ublk::mm
