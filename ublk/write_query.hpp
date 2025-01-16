#pragma once

#include <cstddef>
#include <cstdint>

#include <functional>
#include <memory>
#include <span>
#include <utility>

#include <gsl/assert>

#include "mm/pool_allocators.hpp"

#include "query.hpp"

namespace ublk {

class write_query final : public query {
public:
  template <typename... Args> static auto create(Args &&...args) noexcept {
    return std::allocate_shared<write_query>(
        mm::allocator::pool_cache_line_aligned<write_query>::value,
        std::forward<Args>(args)...);
  }

  write_query() = default;

  write_query(
      std::span<std::byte const> buf, uint64_t offset,
      std::function<void(write_query const &)> &&completer = {}) noexcept
      : buf_(buf), offset_(offset), completer_(std::move(completer)) {
    Ensures(!buf_.empty());
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
    Expects(0 != buf_sz);
    Expects(!(buf_offset + buf_sz > buf_.size()));
    return create(buf_.subspan(buf_offset, buf_sz), rq_offset,
                  std::move(completer));
  }

  auto subquery(uint64_t buf_offset, uint64_t buf_sz, uint64_t rq_offset,
                std::shared_ptr<write_query> parent_wq) const noexcept {
    return subquery(buf_offset, buf_sz, rq_offset,
                    [parent_wq = std::move(parent_wq)](write_query const &wq) {
                      if (wq.err()) [[unlikely]] {
                        parent_wq->set_err(wq.err());
                      }
                    });
  }

  auto buf() const noexcept { return buf_; }

  auto offset() const noexcept { return offset_; }

private:
  std::span<std::byte const> buf_;
  uint64_t offset_;
  std::function<void(write_query const &)> completer_;
};

} // namespace ublk
