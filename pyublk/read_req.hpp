#pragma once

#include <cassert>

#include <memory>
#include <utility>

#include <linux/ublkdrv/cmd.h>

#include "allocators.hpp"
#include "req.hpp"

namespace ublk {

class read_req {
public:
  template <typename... Args> static auto create(Args &&...args) noexcept {
    return std::allocate_shared<read_req>(
        mem::allocator::cache_line_aligned<read_req>::value,
        std::forward<Args>(args)...);
  }

  read_req() = default;

  explicit read_req(std::shared_ptr<req> rq) noexcept : req_(std::move(rq)) {
    assert(req_);
    assert(UBLKDRV_CMD_OP_READ == ublkdrv_cmd_get_op(&req_->cmd()));
  }
  ~read_req() = default;

  read_req(read_req const &) = delete;
  read_req &operator=(read_req const &) = delete;

  read_req(read_req &&) = default;
  read_req &operator=(read_req &&) = default;

  void set_err(int err) noexcept { req_->set_err(err); }
  int err() const noexcept { return req_->err(); }

  auto const &cmd() const noexcept { return req_->cmd().u.r; }

  auto cellds() const noexcept { return req_->cellds(); }
  auto cells() const noexcept { return req_->cells(); }

private:
  std::shared_ptr<req> req_;
};

} // namespace ublk
