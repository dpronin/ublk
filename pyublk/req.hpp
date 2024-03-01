#pragma once

#include <linux/ublkdrv/celld.h>
#include <linux/ublkdrv/cmd.h>

namespace ublk {

class req {
public:
  req() = default;
  explicit req(ublkdrv_cmd const &cmd) noexcept : cmd_(cmd) {}
  virtual ~req() = default;

  req(req const &) = delete;
  req &operator=(req const &) = delete;

  req(req &&other) = delete;
  req &operator=(req &&other) = delete;

  void set_err(int err) noexcept { err_ = err; }
  int err() const noexcept { return err_; }

  auto const &cmd() const noexcept { return cmd_; }

private:
  ublkdrv_cmd cmd_{};
  int err_{0};
};

} // namespace ublk
