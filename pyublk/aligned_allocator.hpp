#pragma once

#include <cstddef>
#include <cstdlib>

#include <algorithm>
#include <limits>
#include <new>

#include "utility.hpp"

namespace ublk {

template <typename T, size_t Alignment = alignof(T)> class aligned_allocator {
public:
  static_assert(is_power_of_2(Alignment), "Alignment must be a power of 2");
  static_assert(alignof(T) <= Alignment,
                "Alignment must be at least alignof(T)");

  template <typename U> struct rebind {
#ifdef __clang__
    using other = aligned_allocator<U, std::max(alignof(U), Alignment)>;
#else
    using other = aligned_allocator<U, Alignment>;
#endif
  };

  using value_type = T;

  aligned_allocator() = default;
  ~aligned_allocator() = default;

  template <typename U, size_t A>
  constexpr explicit aligned_allocator(
      [[maybe_unused]] aligned_allocator<U, A> const &other) noexcept {}

  aligned_allocator(aligned_allocator<T, Alignment> const &other) = default;
  aligned_allocator(aligned_allocator<T, Alignment> &&other) = default;

  T *allocate(size_t n) {
    if (n > std::numeric_limits<size_t>::max() / sizeof(T))
      throw std::bad_array_new_length();

    if (auto *p =
            static_cast<T *>(std::aligned_alloc(Alignment, sizeof(T) * n)))
      return p;

    throw std::bad_alloc();
  }

  void deallocate(T *p, [[maybe_unused]] size_t n) noexcept { std::free(p); }

  template <typename U, size_t A>
  bool operator==(
      [[maybe_unused]] aligned_allocator<U, A> const &other) const noexcept {
    return !(A < Alignment);
  }

  template <typename U, size_t A>
  bool operator!=(aligned_allocator<U, A> const &other) const noexcept {
    return !(*this == other);
  }
};

} // namespace ublk
