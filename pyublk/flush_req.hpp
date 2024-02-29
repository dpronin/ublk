#pragma once

#include <cassert>

#include <memory>
#include <utility>

#include <linux/ublkdrv/cmd.h>

#include "allocators.hpp"
#include "req.hpp"

namespace ublk {

class flush_req {
public:
  template <typename... Args> static auto create(Args &&...args) noexcept {
    return std::allocate_shared<flush_req>(
        mem::allocator::cache_line_aligned<flush_req>::value,
        std::forward<Args>(args)...);
  }

  flush_req() = default;

  explicit flush_req(std::shared_ptr<req> rq) noexcept : req_(std::move(rq)) {
    assert(req_);
    assert(UBLKDRV_CMD_OP_FLUSH == ublkdrv_cmd_get_op(&req_->cmd()));
  }
  ~flush_req() = default;

  flush_req(flush_req const &) = delete;
  flush_req &operator=(flush_req const &) = delete;

  flush_req(flush_req &&) = default;
  flush_req &operator=(flush_req &&) = default;

  void set_err(int err) noexcept { req_->set_err(err); }
  int err() const noexcept { return req_->err(); }

  auto const &cmd() const noexcept { return req_->cmd().u.f; }

private:
  std::shared_ptr<req> req_;
};

} // namespace ublk
