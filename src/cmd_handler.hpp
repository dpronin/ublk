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

#include "flush_handler_interface.hpp"
#include "handler_interface.hpp"
#include "mem.hpp"
#include "read_handler_interface.hpp"
#include "write_handler_interface.hpp"

namespace cfq {

class CmdHandler : public IHandler<int(ublk_cmd) noexcept> {
public:
  explicit CmdHandler(
      std::map<ublk_cmd_op, std::shared_ptr<IHandler<int(ublk_cmd) noexcept>>>
          maphs,
      std::shared_ptr<IHandler<int(ublk_cmd_ack) noexcept>> acknowledger);
  ~CmdHandler() override = default;

  CmdHandler(CmdHandler const &) = default;
  CmdHandler &operator=(CmdHandler const &) = default;

  CmdHandler(CmdHandler &&) = default;
  CmdHandler &operator=(CmdHandler &&) = default;

  int handle(ublk_cmd cmd) noexcept override;

private:
  /* clang-format off */
  std::array<std::shared_ptr<IHandler<int(ublk_cmd) noexcept>>, UBLK_CMD_OP_MAX + 2> hs_;
  /* clang-format on */
  std::shared_ptr<IHandler<int(ublk_cmd_ack) noexcept>> acknowledger_;
};

} // namespace cfq
