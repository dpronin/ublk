#pragma once

#include <cassert>

#include <memory>
#include <utility>

#include <linux/ublkdrv/cmd.h>

#include "mm/pool_allocators.hpp"

#include "cells_holder.hpp"
#include "req.hpp"

namespace ublk {

class read_req final : public req, public cells_holder<false> {
public:
  template <typename... Args> static auto create(Args &&...args) noexcept {
    return std::allocate_shared<read_req>(
        mm::allocator::pool_cache_line_aligned<read_req>::value,
        std::forward<Args>(args)...);
  }

  read_req() = default;
  explicit read_req(
      ublkdrv_cmd const &cmd, std::span<ublkdrv_celld const> cellds,
      std::span<std::byte> cells,
      std::function<void(read_req const &)> &&completer = {}) noexcept
      : req(cmd), cells_holder(cellds, cells),
        completer_(std::move(completer)) {
    assert(UBLKDRV_CMD_OP_READ == ublkdrv_cmd_get_op(&req::cmd()));
  }
  ~read_req() noexcept override {
    static_assert(std::is_nothrow_destructible_v<req>);
    if (completer_) {
      try {
        completer_(*this);
      } catch (...) {
      }
    }
  }

  read_req(read_req const &) = delete;
  read_req &operator=(read_req const &) = delete;

  read_req(read_req &&) = delete;
  read_req &operator=(read_req &&) = delete;

  auto const &cmd() const noexcept { return req::cmd().u.r; }

private:
  std::function<void(read_req const &)> completer_;
};

} // namespace ublk
