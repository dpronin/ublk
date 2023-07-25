#pragma once

#include <cassert>

#include <utility>

#include "target_properties.hpp"
#include "ublk_req_handler_interface.hpp"

namespace ublk {

class Target {
public:
  explicit Target(std::shared_ptr<IUblkReqHandler> handler,
                  target_properties const &props)
      : handler_(std::move(handler)), props_(props) {
    assert(handler_);
  }
  ~Target() = default;

  Target(Target const &) = default;
  Target &operator=(Target const &) = default;

  Target(Target &&) = default;
  Target &operator=(Target &&) = default;

  auto req_handler() const noexcept { return handler_; }
  auto const &properties() const noexcept { return props_; }

private:
  std::shared_ptr<IUblkReqHandler> handler_;
  target_properties props_;
};

} // namespace ublk
