#pragma once

#include <cassert>

#include <memory>
#include <span>
#include <utility>

#include <linux/ublkdrv/celld.h>
#include <linux/ublkdrv/cmd.h>
#include <linux/ublkdrv/cmd_ack.h>

#include <boost/asio/io_context.hpp>

#include "factory_unique_interface.hpp"
#include "handler_interface.hpp"
#include "target_properties.hpp"

namespace ublk {

class Target {
public:
  explicit Target(
      std::unique_ptr<boost::asio::io_context> io_ctx,
      std::shared_ptr<
          IFactoryUnique<IHandler<int(ublkdrv_cmd const &) noexcept>(
              std::span<ublkdrv_celld const>, std::span<std::byte>,
              std::shared_ptr<IHandler<int(ublkdrv_cmd_ack) noexcept>>)>>
          hfactory,
      target_properties const &props)
      : io_ctx_(std::move(io_ctx)), hfactory_(std::move(hfactory)),
        props_(props) {
    assert(io_ctx_);
    assert(hfactory_);
  }
  ~Target() = default;

  Target(Target const &) = delete;
  Target &operator=(Target const &) = delete;

  Target(Target &&) = default;
  Target &operator=(Target &&) = default;

  boost::asio::io_context &io_ctx() noexcept { return *io_ctx_; }
  boost::asio::io_context const &io_ctx() const noexcept { return *io_ctx_; }

  auto const &req_hfactory() const noexcept { return hfactory_; }
  auto const &properties() const noexcept { return props_; }

private:
  std::unique_ptr<boost::asio::io_context> io_ctx_;
  std::shared_ptr<IFactoryUnique<IHandler<int(ublkdrv_cmd const &) noexcept>(
      std::span<ublkdrv_celld const>, std::span<std::byte>,
      std::shared_ptr<IHandler<int(ublkdrv_cmd_ack) noexcept>>)>>
      hfactory_;
  target_properties props_;
};

} // namespace ublk
