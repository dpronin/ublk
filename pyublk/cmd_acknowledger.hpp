#pragma once

#include <memory>

#include <linux/ublkdrv/cmd_ack.h>

#include "handler_interface.hpp"
#include "mem.hpp"
#include "qublkcmd.hpp"

namespace ublk {

class CmdAcknowledger : public IHandler<int(ublkdrv_cmd_ack) noexcept> {
public:
  explicit CmdAcknowledger(std::unique_ptr<qublkcmd_ack_t> qcmd_ack,
                           uptrwd<const int> fd_notify);
  ~CmdAcknowledger() override = default;

  CmdAcknowledger(CmdAcknowledger const &) = delete;
  CmdAcknowledger &operator=(CmdAcknowledger const &) = delete;

  CmdAcknowledger(CmdAcknowledger &&) = default;
  CmdAcknowledger &operator=(CmdAcknowledger &&) = default;

  int handle(ublkdrv_cmd_ack cmd) noexcept override;

private:
  std::unique_ptr<qublkcmd_ack_t> qcmd_ack_;
  uptrwd<const int> fd_notify_;
};

} // namespace ublk
