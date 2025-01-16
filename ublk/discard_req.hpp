#pragma once

#include <memory>
#include <utility>

#include <gsl/assert>

#include <linux/ublkdrv/cmd.h>

#include "mm/pool_allocators.hpp"

#include "req.hpp"

namespace ublk {

class discard_req final : public req {
public:
  template <typename... Args> static auto create(Args &&...args) noexcept {
    return std::allocate_shared<discard_req>(
        mm::allocator::pool_cache_line_aligned<discard_req>::value,
        std::forward<Args>(args)...);
  }

  discard_req() = default;
  explicit discard_req(
      ublkdrv_cmd const &cmd,
      std::function<void(discard_req const &)> &&completer = {}) noexcept
      : req(cmd), completer_(std::move(completer)) {
    Ensures(UBLKDRV_CMD_OP_DISCARD == ublkdrv_cmd_get_op(&req::cmd()));
  }
  ~discard_req() noexcept override {
    static_assert(std::is_nothrow_destructible_v<req>);
    if (completer_) {
      try {
        completer_(*this);
      } catch (...) {
      }
    }
  }

  discard_req(discard_req const &) = delete;
  discard_req &operator=(discard_req const &) = delete;

  discard_req(discard_req &&) = delete;
  discard_req &operator=(discard_req &&) = delete;

  auto const &cmd() const noexcept { return req::cmd().u.d; }

private:
  std::function<void(discard_req const &)> completer_;
};

} // namespace ublk
