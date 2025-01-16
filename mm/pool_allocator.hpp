#pragma once

#include <cstddef>
#include <cstdlib>

#include <algorithm>
#include <memory>
#include <new>

#include <gsl/assert>

#include "utils/utility.hpp"

#include "mem_cached_allocator.hpp"
#include "mem_pool_allocator.hpp"

namespace ublk::mm {

template <typename T, size_t Alignment = alignof(T)> class pool_allocator {
  static_assert(is_power_of_2(Alignment), "Alignment must be a power of 2");
  static_assert(alignof(T) <= Alignment,
                "Alignment must be at least alignof(T)");

private:
  static inline auto __mpool_a__ =
      std::make_shared<mem_pool_allocator<T, Alignment>>(
          __mem_cached_allocator__);

public:
  template <typename U> struct rebind {
#ifdef __clang__
    using other = pool_allocator<U, std::max(alignof(U), Alignment)>;
#else
    using other = pool_allocator<U, Alignment>;
#endif
  };

  using value_type = T;

  explicit pool_allocator() = default;
  ~pool_allocator() = default;

  pool_allocator(pool_allocator<T, Alignment> const &other) = default;
  pool_allocator &
  operator=(pool_allocator<T, Alignment> const &other) = default;

  pool_allocator(pool_allocator<T, Alignment> &&other) = default;
  pool_allocator &operator=(pool_allocator<T, Alignment> &&other) = default;

  template <typename U, size_t A>
  constexpr explicit pool_allocator(
      [[maybe_unused]] pool_allocator<U, A> const &other) noexcept {}

  T *allocate(size_t n [[maybe_unused]]) {
    Expects(1 == n);
    if (auto *p = static_cast<T *>(__mpool_a__->allocate()))
      return p;
    throw std::bad_alloc();
  }

  void deallocate(T *p, [[maybe_unused]] size_t n) noexcept {
    Expects(1 == n);
    __mpool_a__->free(p);
  }

  template <typename U, size_t A>
  bool operator==(
      [[maybe_unused]] pool_allocator<U, A> const &other) const noexcept {
    return !(A < Alignment);
  }

  template <typename U, size_t A>
  bool operator!=(pool_allocator<U, A> const &other) const noexcept {
    return !(*this == other);
  }
};

} // namespace ublk::mm
