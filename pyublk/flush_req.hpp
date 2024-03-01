#pragma once

#include <cassert>

#include <memory>
#include <utility>

#include <linux/ublkdrv/cmd.h>

#include "mm/pool_allocators.hpp"

#include "req.hpp"

namespace ublk {

class flush_req final : public req {
public:
  template <typename... Args> static auto create(Args &&...args) noexcept {
    return std::allocate_shared<flush_req>(
        mm::allocator::pool_cache_line_aligned<flush_req>::value,
        std::forward<Args>(args)...);
  }

  flush_req() = default;
  explicit flush_req(
      ublkdrv_cmd const &cmd,
      std::function<void(flush_req const &)> &&completer = {}) noexcept
      : req(cmd), completer_(std::move(completer)) {
    assert(UBLKDRV_CMD_OP_FLUSH == ublkdrv_cmd_get_op(&req::cmd()));
  }
  ~flush_req() noexcept override {
    static_assert(std::is_nothrow_destructible_v<req>);
    if (completer_) {
      try {
        completer_(*this);
      } catch (...) {
      }
    }
  }

  flush_req(flush_req const &) = delete;
  flush_req &operator=(flush_req const &) = delete;

  flush_req(flush_req &&) = delete;
  flush_req &operator=(flush_req &&) = delete;

  auto const &cmd() const noexcept { return req::cmd().u.f; }

private:
  std::function<void(flush_req const &)> completer_;
};

} // namespace ublk
