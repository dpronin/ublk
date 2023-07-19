#pragma once

#include <memory>

#include <linux/ublk/cmd_ack.h>

#include "cmd_handler_interface.hpp"
#include "mem.hpp"
#include "qublkcmd.hpp"

namespace cfq {

class CmdAcknowledger : public ICmdHandler<const ublk_cmd_ack> {
public:
  explicit CmdAcknowledger(std::unique_ptr<qublkcmd_ack_t> qcmd_ack,
                           uptrwd<const int> fd_notify);
  ~CmdAcknowledger() override = default;

  CmdAcknowledger(CmdAcknowledger const &) = delete;
  CmdAcknowledger &operator=(CmdAcknowledger const &) = delete;

  CmdAcknowledger(CmdAcknowledger &&) = default;
  CmdAcknowledger &operator=(CmdAcknowledger &&) = default;

  int handle(ublk_cmd_ack const &cmd) noexcept override;

private:
  std::unique_ptr<qublkcmd_ack_t> qcmd_ack_;
  uptrwd<const int> fd_notify_;
};

} // namespace cfq
