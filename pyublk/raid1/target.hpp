#pragma once

#include <cassert>

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "rw_handler_interface.hpp"

namespace ublk::raid1 {

class Target final {
public:
  explicit Target(std::vector<std::shared_ptr<IRWHandler>> hs) noexcept
      : next_id_(0), hs_(std::move(hs)) {
    assert(!(hs_.size() < 2));
    assert(next_id_ < hs_.size());
    assert(std::ranges::all_of(
        hs_, [](auto const &h) { return static_cast<bool>(h); }));
  }

  int process(std::shared_ptr<read_query> rq) noexcept {
    if (auto const res = hs_[next_id_]->submit(std::move(rq)); !res) [[likely]]
      next_id_ = (next_id_ + 1) % hs_.size();
    return 0;
  }

  int process(std::shared_ptr<write_query> wq) noexcept {
    for (auto const &h : hs_) {
      if (auto const res = h->submit(wq)) [[unlikely]] {
        return res;
      }
    }
    return 0;
  }

private:
  uint32_t next_id_;
  std::vector<std::shared_ptr<IRWHandler>> hs_;
};

} // namespace ublk::raid1
