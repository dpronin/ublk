#pragma once

#include <cstddef>

#include <linux/ublk/cellc.h>
#include <linux/ublk/cmd.h>
#include <linux/ublk/cmd_ack.h>

#include <memory>
#include <span>

#include "handler_interface.hpp"
#include "ublk_req_handler_interface.hpp"

namespace ublk {

class CmdHandler : public IHandler<int(ublk_cmd) noexcept> {
public:
  explicit CmdHandler(
      std::shared_ptr<IUblkReqHandler> handler,
      std::shared_ptr<ublk_cellc const> cellc, std::span<std::byte> cells,
      std::shared_ptr<IHandler<int(ublk_cmd_ack) noexcept>> acknowledger);
  ~CmdHandler() override = default;

  CmdHandler(CmdHandler const &) = default;
  CmdHandler &operator=(CmdHandler const &) = default;

  CmdHandler(CmdHandler &&) = default;
  CmdHandler &operator=(CmdHandler &&) = default;

  int handle(ublk_cmd cmd) noexcept override;

private:
  std::shared_ptr<IUblkReqHandler> handler_;
  std::shared_ptr<ublk_cellc const> cellc_;
  std::span<std::byte> cells_;
  std::shared_ptr<IHandler<int(ublk_cmd_ack) noexcept>> acknowledger_;
};

} // namespace ublk
