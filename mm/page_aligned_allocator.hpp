#pragma once

#include <cstddef>
#include <cstdlib>

#include <algorithm>
#include <limits>
#include <memory>
#include <new>
#include <utility>

#include <gsl/assert>

#include "sys/page.hpp"

#include "mem_allocator.hpp"

namespace ublk::mm {

template <typename T = void> class page_aligned_allocator {
public:
  template <typename U> struct rebind {
    using other = page_aligned_allocator<U>;
  };

  using value_type = T;

  explicit page_aligned_allocator(std::shared_ptr<mem_allocator> a = {})
      : a_(std::move(a)) {
    if (!a_)
      a_ = std::make_shared<mem_allocator>();
  }
  ~page_aligned_allocator() = default;

  template <typename U>
  constexpr explicit page_aligned_allocator(
      [[maybe_unused]] page_aligned_allocator<U> const &other) noexcept
      : a_(other.underlying_allocator()) {
    Ensures(a_);
  }

  page_aligned_allocator(page_aligned_allocator<T> const &) = default;
  page_aligned_allocator(page_aligned_allocator<T> &&) = default;

  T *allocate(size_t n) {
    if (n > std::numeric_limits<size_t>::max() / sizeof(T))
      throw std::bad_array_new_length();

    if (auto const alignment = std::max(sys::page_size(), alignof(T));
        alignment > 0) {
      if (auto *p =
              static_cast<T *>(a_->allocate_aligned(alignment, sizeof(T) * n)))
        return p;
    }

    throw std::bad_alloc();
  }

  void deallocate(T *p, size_t n [[maybe_unused]]) noexcept { a_->free(p); }

  auto const &underlying_allocator() const noexcept { return a_; }

private:
  std::shared_ptr<mem_allocator> a_;
};

} // namespace ublk::mm
