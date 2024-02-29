#pragma once

#include <functional>
#include <memory>
#include <utility>

#include "allocators.hpp"
#include "query.hpp"

namespace ublk {

class flush_query final : public query {
public:
  template <typename... Args> static auto create(Args &&...args) noexcept {
    return std::allocate_shared<flush_query>(
        mem::allocator::cache_line_aligned<flush_query>::value,
        std::forward<Args>(args)...);
  }

  flush_query() = default;

  flush_query(
      std::function<void(flush_query const &)> &&completer = {}) noexcept
      : completer_(std::move(completer)) {}

  ~flush_query() {
    if (completer_) {
      try {
        completer_(*this);
      } catch (...) {
      }
    }
  }

  flush_query(flush_query const &) = delete;
  flush_query &operator=(flush_query const &) = delete;

  flush_query(flush_query &&) = default;
  flush_query &operator=(flush_query &&) = default;

private:
  std::function<void(flush_query const &)> completer_;
};

} // namespace ublk
