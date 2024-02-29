#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>

#include <functional>
#include <memory>
#include <span>
#include <utility>

#include "allocators.hpp"
#include "query.hpp"

namespace ublk {

class write_query final : public query {
public:
  template <typename... Args> static auto create(Args &&...args) noexcept {
    return std::allocate_shared<write_query>(
        mem::allocator::cache_line_aligned<write_query>::value,
        std::forward<Args>(args)...);
  }

  write_query() = default;

  write_query(
      std::span<std::byte const> buf, uint64_t offset,
      std::function<void(write_query const &)> &&completer = {}) noexcept
      : buf_(buf), offset_(offset), completer_(std::move(completer)) {
    assert(!buf_.empty());
  }
  ~write_query() {
    if (completer_) {
      try {
        completer_(*this);
      } catch (...) {
      }
    }
  }

  write_query(write_query const &) = delete;
  write_query &operator=(write_query const &) = delete;

  write_query(write_query &&) = default;
  write_query &operator=(write_query &&) = default;

  auto subquery(uint64_t buf_offset, uint64_t buf_sz, uint64_t rq_offset,
                std::function<void(write_query const &)> &&completer = {})
      const noexcept {
    return create(buf_.subspan(buf_offset, buf_sz), rq_offset,
                  std::move(completer));
  }

  auto buf() const noexcept { return buf_; }

  auto offset() const noexcept { return offset_; }

private:
  std::span<std::byte const> buf_;
  uint64_t offset_;
  std::function<void(write_query const &)> completer_;
};

} // namespace ublk
