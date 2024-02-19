#pragma once

#include <cstddef>

#include <linux/ublkdrv/celld.h>
#include <linux/ublkdrv/cmd.h>
#include <linux/ublkdrv/cmd_ack.h>

#include <memory>
#include <span>

#include "handler_interface.hpp"
#include "ublk_req_handler_interface.hpp"

namespace ublk {

class CmdHandler : public IHandler<int(ublkdrv_cmd) noexcept> {
public:
  explicit CmdHandler(
      std::shared_ptr<IUblkReqHandler> handler,
      std::span<ublkdrv_celld const> cellds, std::span<std::byte> cells,
      std::shared_ptr<IHandler<int(ublkdrv_cmd_ack) noexcept>> acknowledger);
  ~CmdHandler() override = default;

  CmdHandler(CmdHandler const &) = default;
  CmdHandler &operator=(CmdHandler const &) = default;

  CmdHandler(CmdHandler &&) = default;
  CmdHandler &operator=(CmdHandler &&) = default;

  int handle(ublkdrv_cmd cmd) noexcept override;

private:
  std::shared_ptr<IUblkReqHandler> handler_;
  std::span<ublkdrv_celld const> cellds_;
  std::span<std::byte> cells_;
  std::shared_ptr<IHandler<int(ublkdrv_cmd_ack) noexcept>> acknowledger_;
};

} // namespace ublk
