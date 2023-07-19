#pragma once

#include <cstddef>

#include <memory>
#include <span>

#include <linux/ublk/cellc.h>
#include <linux/ublk/cmd.h>
#include <linux/ublk/cmd_ack.h>

#include "cmd_flush_handler.hpp"
#include "cmd_flush_handler_adaptor.hpp"
#include "cmd_handler.hpp"
#include "cmd_read_handler.hpp"
#include "cmd_read_handler_adaptor.hpp"
#include "cmd_write_handler.hpp"
#include "cmd_write_handler_adaptor.hpp"
#include "factory_unique_interface.hpp"
#include "flush_handler_interface.hpp"
#include "handler_interface.hpp"
#include "mem.hpp"
#include "read_handler_interface.hpp"
#include "rvwrap.hpp"
#include "write_handler_interface.hpp"

namespace cfq {

class CmdHandlerFactory
    : public IFactoryUnique<rvwrap<IHandler<int(ublk_cmd) noexcept>>(
          std::shared_ptr<ublk_cellc const>, std::span<std::byte>,
          std::shared_ptr<IReadHandler>, std::shared_ptr<IWriteHandler>,
          std::shared_ptr<IFlushHandler>,
          std::shared_ptr<IHandler<int(ublk_cmd_ack) noexcept>>)> {
public:
  CmdHandlerFactory() = default;
  ~CmdHandlerFactory() override = default;

  CmdHandlerFactory(CmdHandlerFactory const &) = default;
  CmdHandlerFactory &operator=(CmdHandlerFactory const &) = default;

  CmdHandlerFactory(CmdHandlerFactory &&) = default;
  CmdHandlerFactory &operator=(CmdHandlerFactory &&) = default;

  std::unique_ptr<IHandler<int(ublk_cmd) noexcept>> create_unique(
      std::shared_ptr<ublk_cellc const> cellc, std::span<std::byte> cells,
      std::shared_ptr<IReadHandler> reader,
      std::shared_ptr<IWriteHandler> writer,
      std::shared_ptr<IFlushHandler> flusher,
      std::shared_ptr<IHandler<int(ublk_cmd_ack) noexcept>> acknowledger)
      override {

    auto hs =
        std::map<ublk_cmd_op,
                 std::shared_ptr<IHandler<int(ublk_cmd) noexcept>>>{
            {
                UBLK_CMD_OP_READ,
                std::make_unique<CmdReadHandlerAdaptor>(
                    std::make_unique<CmdReadHandler>(cellc, cells,
                                                     std::move(reader))),
            },
            {
                UBLK_CMD_OP_WRITE,
                std::make_unique<CmdWriteHandlerAdaptor>(
                    std::make_unique<CmdWriteHandler>(cellc, cells,
                                                      std::move(writer))),
            },
            {
                UBLK_CMD_OP_FLUSH,
                std::make_unique<CmdFlushHandlerAdaptor>(
                    std::make_unique<CmdFlushHandler>(std::move(flusher))),
            },
        };
    return std::make_unique<CmdHandler>(std::move(hs), std::move(acknowledger));
  }
};

} // namespace cfq
