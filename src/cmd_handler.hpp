#pragma once

#include <cstddef>

#include <array>
#include <map>
#include <memory>
#include <span>
#include <utility>

#include <linux/ublk/cellc.h>
#include <linux/ublk/cmd.h>
#include <linux/ublk/cmd_ack.h>

#include "cmd_handler_interface.hpp"
#include "flush_handler_interface.hpp"
#include "mem.hpp"
#include "read_handler_interface.hpp"
#include "write_handler_interface.hpp"

namespace cfq {

class CmdHandler : public ICmdHandler<const ublk_cmd> {
public:
  explicit CmdHandler(
      std::map<ublk_cmd_op, std::shared_ptr<ICmdHandler<const ublk_cmd>>> maphs,
      std::shared_ptr<ICmdHandler<const ublk_cmd_ack>> acknowledger);
  ~CmdHandler() override = default;

  CmdHandler(CmdHandler const &) = delete;
  CmdHandler &operator=(CmdHandler const &) = delete;

  CmdHandler(CmdHandler &&) = default;
  CmdHandler &operator=(CmdHandler &&) = default;

  int handle(ublk_cmd const &cmd) noexcept override;

private:
  /* clang-format off */
  std::array<std::shared_ptr<ICmdHandler<const ublk_cmd>>, UBLK_CMD_OP_WRITE_ZEROES + 2> hs_;
  /* clang-format on */
  std::shared_ptr<ICmdHandler<const ublk_cmd_ack>> acknowledger_;
};

} // namespace cfq
