#pragma once

#include <cstddef>

#include <functional>
#include <span>
#include <utility>

#include <linux/ublkdrv/celld.h>
#include <linux/ublkdrv/cmd.h>

#include "allocators.hpp"

namespace ublk {

class ublk_req {
public:
  template <typename... Args> static auto create(Args &&...args) {
    return std::allocate_shared<ublk_req>(
        mem::allocator::cache_line_aligned<ublk_req>::value,
        std::forward<Args>(args)...);
  }

  ublk_req() = default;

  explicit ublk_req(
      ublkdrv_cmd cmd, std::span<ublkdrv_celld const> cellds,
      std::span<std::byte> cells,
      std::function<void(ublk_req const &)> &&completer = {}) noexcept
      : cmd_(cmd), cellds_(cellds), cells_(cells), err_(0),
        completer_(std::move(completer)) {}

  ~ublk_req() noexcept {
    if (completer_) {
      try {
        completer_(*this);
      } catch (...) {
      }
    }
  }

  ublk_req(ublk_req const &) = delete;
  ublk_req &operator=(ublk_req const &) = delete;

  ublk_req(ublk_req &&other) noexcept = default;
  ublk_req &operator=(ublk_req &&other) noexcept = default;

  void set_err(int err) noexcept { err_ = err; }
  int err() const noexcept { return err_; }

  auto const &cmd() const noexcept { return cmd_; }

  auto cellds() const noexcept { return cellds_; }
  auto cells() const noexcept { return cells_; }

private:
  ublkdrv_cmd cmd_{};
  std::span<ublkdrv_celld const> cellds_;
  std::span<std::byte> cells_;
  int err_;
  std::function<void(ublk_req const &)> completer_;
};

} // namespace ublk
