#pragma once

#include <cstddef>

#include <array>
#include <map>
#include <memory>
#include <span>

#include <linux/ublkdrv/celld.h>
#include <linux/ublkdrv/cmd.h>
#include <linux/ublkdrv/cmd_ack.h>

#include "handler_interface.hpp"
#include "ublk_req_handler_interface.hpp"

namespace ublk {

class CmdHandler : public IHandler<int(ublkdrv_cmd const &) noexcept> {
public:
  explicit CmdHandler(
      std::span<ublkdrv_celld const> cellds, std::span<std::byte> cells,
      std::shared_ptr<IHandler<int(ublkdrv_cmd_ack) noexcept>> acknowledger,
      std::map<ublkdrv_cmd_op, std::shared_ptr<IUblkReqHandler>> const &maphs);
  ~CmdHandler() override = default;

  CmdHandler(CmdHandler const &) = default;
  CmdHandler &operator=(CmdHandler const &) = default;

  CmdHandler(CmdHandler &&) = default;
  CmdHandler &operator=(CmdHandler &&) = default;

  int handle(ublkdrv_cmd const &cmd) noexcept override;

private:
  std::array<std::shared_ptr<IUblkReqHandler>, UBLKDRV_CMD_OP_MAX + 2> hs_;
  std::span<ublkdrv_celld const> cellds_;
  std::span<std::byte> cells_;
  std::shared_ptr<IHandler<int(ublkdrv_cmd_ack) noexcept>> acknowledger_;
};

} // namespace ublk
