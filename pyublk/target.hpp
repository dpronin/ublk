#pragma once

#include <cassert>

#include <memory>
#include <utility>

#include <boost/asio/io_context.hpp>

#include "target_properties.hpp"
#include "ublk_req_handler_interface.hpp"

namespace ublk {

class Target {
public:
  explicit Target(std::unique_ptr<boost::asio::io_context> io_ctx,
                  std::shared_ptr<IUblkReqHandler> handler,
                  target_properties const &props)
      : io_ctx_(std::move(io_ctx)), handler_(std::move(handler)),
        props_(props) {
    assert(io_ctx_);
    assert(handler_);
  }
  ~Target() = default;

  Target(Target const &) = delete;
  Target &operator=(Target const &) = delete;

  Target(Target &&) = default;
  Target &operator=(Target &&) = default;

  boost::asio::io_context &io_ctx() noexcept { return *io_ctx_; }
  boost::asio::io_context const &io_ctx() const noexcept { return *io_ctx_; }

  auto req_handler() const noexcept { return handler_; }
  auto const &properties() const noexcept { return props_; }

private:
  std::unique_ptr<boost::asio::io_context> io_ctx_;
  std::shared_ptr<IUblkReqHandler> handler_;
  target_properties props_;
};

} // namespace ublk
