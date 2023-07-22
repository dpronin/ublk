#pragma once

#include <cassert>
#include <cstddef>

#include <functional>
#include <span>
#include <utility>

#include <linux/ublk/cellc.h>
#include <linux/ublk/cmd.h>

namespace cfq {

class ublk_req {
public:
  explicit ublk_req(
      ublk_cmd cmd, std::shared_ptr<ublk_cellc const> cellc,
      std::span<std::byte> cells,
      std::function<void(ublk_cmd const &, int)> &&completer) noexcept
      : cmd_(cmd), cellc_(std::move(cellc)), cells_(cells), err_(0),
        completer_(std::move(completer)) {

    assert(cellc_);
  }

  ~ublk_req() noexcept {
    if (completer_)
      completer_(cmd_, err_);
  }

  ublk_req(ublk_req const &) = delete;
  ublk_req &operator=(ublk_req const &) = delete;

  ublk_req(ublk_req &&other) noexcept
      : cmd_(other.cmd_), err_(other.err_),
        completer_(std::exchange(other.completer_, nullptr)) {}

  ublk_req &operator=(ublk_req &&other) noexcept {
    cmd_ = other.cmd_;
    err_ = other.err_;
    completer_ = std::exchange(other.completer_, nullptr);
    return *this;
  }

  void set_err(int err) noexcept { err_ = err; }
  int err() const noexcept { return err_; }

  auto const &cmd() const noexcept { return cmd_; }

  auto const &cellc() const noexcept { return *cellc_; }
  auto cells() const noexcept { return cells_; }

private:
  ublk_cmd cmd_;
  std::shared_ptr<ublk_cellc const> cellc_;
  std::span<std::byte> cells_;
  int err_;
  std::function<void(ublk_cmd const &, int)> completer_;
};

} // namespace cfq
