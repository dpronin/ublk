#pragma once

#include <cstdint>

#include <functional>
#include <memory>
#include <utility>

#include "mm/pool_allocators.hpp"

#include "query.hpp"

namespace ublk {

class discard_query final : public query {
public:
  template <typename... Args> static auto create(Args &&...args) noexcept {
    return std::allocate_shared<discard_query>(
        mm::allocator::pool_cache_line_aligned<discard_query>::value,
        std::forward<Args>(args)...);
  }

  discard_query() = default;

  discard_query(
      uint64_t offset, uint64_t sz,
      std::function<void(discard_query const &)> &&completer = {}) noexcept
      : offset_(offset), sz_(sz), completer_(std::move(completer)) {}
  ~discard_query() {
    if (completer_) {
      try {
        completer_(*this);
      } catch (...) {
      }
    }
  }

  discard_query(discard_query const &) = delete;
  discard_query &operator=(discard_query const &) = delete;

  discard_query(discard_query &&) = default;
  discard_query &operator=(discard_query &&) = default;

  std::shared_ptr<discard_query>
  subquery(uint64_t rq_offset, uint64_t rq_sz,
           std::function<void(discard_query const &)> &&completer = {})
      const noexcept {
    return create(rq_offset, rq_sz, std::move(completer));
  }

  auto size() const noexcept { return sz_; }
  auto offset() const noexcept { return offset_; }

private:
  uint64_t offset_;
  uint64_t sz_;
  std::function<void(discard_query const &)> completer_;
};

} // namespace ublk
