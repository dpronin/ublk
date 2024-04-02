#pragma once

#include <cstddef>

#include <map>
#include <memory>
#include <span>

#include <linux/ublkdrv/celld.h>
#include <linux/ublkdrv/cmd.h>
#include <linux/ublkdrv/cmd_ack.h>

#include "cmd_handler.hpp"
#include "factory_unique_interface.hpp"
#include "handler_interface.hpp"
#include "ublk_req_handler_interface.hpp"

namespace ublk {

class CmdHandlerFactory
    : public IFactoryUnique<IHandler<int(ublkdrv_cmd const &) noexcept>(
          std::span<ublkdrv_celld const>, std::span<std::byte>,
          std::shared_ptr<IHandler<int(ublkdrv_cmd_ack) noexcept>>)> {
public:
  explicit CmdHandlerFactory(
      std::map<ublkdrv_cmd_op, std::shared_ptr<IUblkReqHandler>> maphs)
      : maphs_(std::move(maphs)) {}
  ~CmdHandlerFactory() override = default;

  CmdHandlerFactory(CmdHandlerFactory const &) = default;
  CmdHandlerFactory &operator=(CmdHandlerFactory const &) = default;

  CmdHandlerFactory(CmdHandlerFactory &&) = default;
  CmdHandlerFactory &operator=(CmdHandlerFactory &&) = default;

  std::unique_ptr<IHandler<int(ublkdrv_cmd const &) noexcept>> create_unique(
      std::span<ublkdrv_celld const> cellds, std::span<std::byte> cells,
      std::shared_ptr<IHandler<int(ublkdrv_cmd_ack) noexcept>> acknowledger)
      override {
    return std::make_unique<CmdHandler>(cellds, cells, std::move(acknowledger),
                                        maphs_);
  }

private:
  std::map<ublkdrv_cmd_op, std::shared_ptr<IUblkReqHandler>> maphs_;
};

} // namespace ublk
