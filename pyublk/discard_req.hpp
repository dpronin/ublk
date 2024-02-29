#pragma once

#include <cassert>

#include <memory>
#include <utility>

#include <linux/ublkdrv/cmd.h>

#include "allocators.hpp"
#include "req.hpp"

namespace ublk {

class discard_req {
public:
  template <typename... Args> static auto create(Args &&...args) noexcept {
    return std::allocate_shared<discard_req>(
        mem::allocator::cache_line_aligned<discard_req>::value,
        std::forward<Args>(args)...);
  }

  discard_req() = default;

  explicit discard_req(std::shared_ptr<req> rq) noexcept : req_(std::move(rq)) {
    assert(req_);
    assert(UBLKDRV_CMD_OP_DISCARD == ublkdrv_cmd_get_op(&req_->cmd()));
  }
  ~discard_req() = default;

  discard_req(discard_req const &) = delete;
  discard_req &operator=(discard_req const &) = delete;

  discard_req(discard_req &&) = default;
  discard_req &operator=(discard_req &&) = default;

  void set_err(int err) noexcept { req_->set_err(err); }
  int err() const noexcept { return req_->err(); }

  auto const &cmd() const noexcept { return req_->cmd().u.d; }

private:
  std::shared_ptr<req> req_;
};

} // namespace ublk
