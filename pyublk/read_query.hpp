#pragma once

#include <cstddef>
#include <cstdint>

#include <functional>
#include <memory>
#include <span>
#include <utility>

#include "pool_allocators.hpp"
#include "query.hpp"

namespace ublk {

class read_query final : public query {
public:
  template <typename... Args> static auto create(Args &&...args) noexcept {
    return std::allocate_shared<read_query>(
        mem::allocator::pool_cache_line_aligned<read_query>::value,
        std::forward<Args>(args)...);
  }

  read_query() = default;

  read_query(std::span<std::byte> buf, uint64_t offset,
             std::function<void(read_query const &)> &&completer = {}) noexcept
      : buf_(buf), offset_(offset), completer_(std::move(completer)) {
    assert(!buf_.empty());
  }
  ~read_query() {
    if (completer_) {
      try {
        completer_(*this);
      } catch (...) {
      }
    }
  }

  read_query(read_query const &) = delete;
  read_query &operator=(read_query const &) = delete;

  read_query(read_query &&) = default;
  read_query &operator=(read_query &&) = default;

  auto subquery(
      uint64_t buf_offset, uint64_t buf_sz, uint64_t rq_offset,
      std::function<void(read_query const &)> &&completer = {}) const noexcept {
    return create(buf_.subspan(buf_offset, buf_sz), rq_offset,
                  std::move(completer));
  }

  auto buf() noexcept { return buf_; }
  auto buf() const noexcept { return std::span<std::byte const>{buf_}; }

  auto offset() const noexcept { return offset_; }

private:
  std::span<std::byte> buf_;
  uint64_t offset_;
  std::function<void(read_query const &)> completer_;
};

} // namespace ublk
