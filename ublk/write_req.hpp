#pragma once

#include <memory>
#include <type_traits>
#include <utility>

#include <gsl/assert>

#include <linux/ublkdrv/cmd.h>

#include "mm/pool_allocators.hpp"

#include "cells_holder.hpp"
#include "req.hpp"

namespace ublk {

class write_req final : public req, public cells_holder<true> {
public:
  template <typename... Args> static auto create(Args &&...args) noexcept {
    return std::allocate_shared<write_req>(
        mm::allocator::pool_cache_line_aligned<write_req>::value,
        std::forward<Args>(args)...);
  }

  write_req() = default;
  explicit write_req(
      ublkdrv_cmd const &cmd, std::span<ublkdrv_celld const> cellds,
      std::span<std::byte> cells,
      std::function<void(write_req const &)> &&completer = {}) noexcept
      : req(cmd), cells_holder(cellds, cells),
        completer_(std::move(completer)) {
    Ensures(UBLKDRV_CMD_OP_WRITE == ublkdrv_cmd_get_op(&req::cmd()));
  }
  ~write_req() noexcept override {
    static_assert(std::is_nothrow_destructible_v<req>);
    if (completer_) {
      try {
        completer_(*this);
      } catch (...) {
      }
    }
  }

  write_req(write_req const &) = delete;
  write_req &operator=(write_req const &) = delete;

  write_req(write_req &&) = delete;
  write_req &operator=(write_req &&) = delete;

  auto const &cmd() const noexcept { return req::cmd().u.w; }

private:
  std::function<void(write_req const &)> completer_;
};

} // namespace ublk
