#pragma once

#include <cassert>

#include <memory>
#include <utility>

#include <linux/ublkdrv/cmd.h>

#include "allocators.hpp"
#include "req.hpp"

namespace ublk {

class write_req {
public:
  template <typename... Args> static auto create(Args &&...args) noexcept {
    return std::allocate_shared<write_req>(
        mem::allocator::cache_line_aligned<write_req>::value,
        std::forward<Args>(args)...);
  }

  write_req() = default;

  explicit write_req(std::shared_ptr<req> rq) noexcept : req_(std::move(rq)) {
    assert(req_);
    assert(UBLKDRV_CMD_OP_WRITE == ublkdrv_cmd_get_op(&req_->cmd()));
  }
  ~write_req() = default;

  write_req(write_req const &) = delete;
  write_req &operator=(write_req const &) = delete;

  write_req(write_req &&) = default;
  write_req &operator=(write_req &&) = default;

  void set_err(int err) noexcept { req_->set_err(err); }
  int err() const noexcept { return req_->err(); }

  auto const &cmd() const noexcept { return req_->cmd().u.w; }

  auto cellds() const noexcept { return req_->cellds(); }
  std::span<std::byte const> cells() const noexcept { return req_->cells(); }

private:
  std::shared_ptr<req> req_;
};

} // namespace ublk
