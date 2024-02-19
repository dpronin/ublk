#pragma once

#include <cstddef>

#include <memory>
#include <span>

#include <linux/ublkdrv/celld.h>
#include <linux/ublkdrv/cmd.h>
#include <linux/ublkdrv/cmd_ack.h>

#include "cmd_handler.hpp"
#include "factory_unique_interface.hpp"
#include "handler_interface.hpp"
#include "rvwrap.hpp"
#include "ublk_req_handler_interface.hpp"

namespace ublk {

class CmdHandlerFactory
    : public IFactoryUnique<rvwrap<IHandler<int(ublkdrv_cmd) noexcept>>(
          std::shared_ptr<IUblkReqHandler>, std::span<ublkdrv_celld const>,
          std::span<std::byte>,
          std::shared_ptr<IHandler<int(ublkdrv_cmd_ack) noexcept>>)> {
public:
  CmdHandlerFactory() = default;
  ~CmdHandlerFactory() override = default;

  CmdHandlerFactory(CmdHandlerFactory const &) = default;
  CmdHandlerFactory &operator=(CmdHandlerFactory const &) = default;

  CmdHandlerFactory(CmdHandlerFactory &&) = default;
  CmdHandlerFactory &operator=(CmdHandlerFactory &&) = default;

  std::unique_ptr<IHandler<int(ublkdrv_cmd) noexcept>>
  create_unique(std::shared_ptr<IUblkReqHandler> handler,
                std::span<ublkdrv_celld const> cellds, std::span<std::byte> cells,
                std::shared_ptr<IHandler<int(ublkdrv_cmd_ack) noexcept>>
                    acknowledger) override {

    return std::make_unique<CmdHandler>(std::move(handler), cellds, cells,
                                        std::move(acknowledger));
  }
};

} // namespace ublk
