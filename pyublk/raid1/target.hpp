#pragma once

#include <cassert>

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "rw_handler_interface.hpp"

namespace ublk::raid1 {

class Target {
public:
  explicit Target(std::vector<std::shared_ptr<IRWHandler>> hs)
      : next_id_(0), hs_(std::move(hs)) {
    assert(!(hs_.size() < 2));
    assert(next_id_ < hs_.size());
    assert(std::ranges::all_of(
        hs_, [](auto const &h) { return static_cast<bool>(h); }));
  }
  virtual ~Target() = default;

  Target(Target const &) = default;
  Target &operator=(Target const &) = default;

  Target(Target &&) = default;
  Target &operator=(Target &&) = default;

  virtual ssize_t read(std::span<std::byte> buf, __off64_t offset) noexcept {
    auto const res = hs_[next_id_]->read(buf, offset);
    if (res > 0) [[likely]]
      next_id_ = (next_id_ + 1) % hs_.size();
    return res;
  }

  virtual ssize_t write(std::span<std::byte const> buf,
                        __off64_t offset) noexcept {
    auto rb{static_cast<ssize_t>(buf.size())};

    for (auto const &h : hs_) {
      if (auto const res = h->write(buf, offset); res >= 0) [[likely]] {
        rb = std::min(rb, res);
      } else {
        return res;
      }
    }

    return rb;
  }

private:
  uint32_t next_id_;
  std::vector<std::shared_ptr<IRWHandler>> hs_;
};

} // namespace ublk::raid1
