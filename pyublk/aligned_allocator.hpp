#pragma once

#include <cassert>
#include <cstddef>
#include <cstdlib>

#include <algorithm>
#include <limits>
#include <memory>
#include <new>

#include "mem_allocator.hpp"
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

  explicit aligned_allocator(std::shared_ptr<mem_allocator> a = {})
      : a_(std::move(a)) {
    if (!a_)
      a_ = std::make_shared<mem_allocator>();
  }
  ~aligned_allocator() = default;

  template <typename U, size_t A>
  constexpr explicit aligned_allocator(
      [[maybe_unused]] aligned_allocator<U, A> const &other) noexcept
      : a_(other.underlying_allocator()) {
    assert(a_);
  }

  aligned_allocator(aligned_allocator<T, Alignment> const &other) = default;
  aligned_allocator(aligned_allocator<T, Alignment> &&other) = default;

  T *allocate(size_t n) {
    if (n > std::numeric_limits<size_t>::max() / sizeof(T))
      throw std::bad_array_new_length();

    if (auto *p =
            static_cast<T *>(a_->allocate_aligned(Alignment, sizeof(T) * n)))
      return p;

    throw std::bad_alloc();
  }

  void deallocate(T *p, [[maybe_unused]] size_t n) noexcept {
    a_->free_aligned(Alignment, p);
  }

  template <typename U, size_t A>
  bool operator==(
      [[maybe_unused]] aligned_allocator<U, A> const &other) const noexcept {
    return !(A < Alignment);
  }

  template <typename U, size_t A>
  bool operator!=(aligned_allocator<U, A> const &other) const noexcept {
    return !(*this == other);
  }

  auto const &underlying_allocator() const noexcept { return a_; }

private:
  std::shared_ptr<mem_allocator> a_;
};

} // namespace ublk
