#pragma once

#include <memory>

#include <linux/ublkdrv/cmd_ack.h>

#include "mm/mem_types.hpp"

#include "handler_interface.hpp"
#include "qublkcmd.hpp"

namespace ublk {

class CmdAcknowledger : public IHandler<int(ublkdrv_cmd_ack) noexcept> {
public:
  explicit CmdAcknowledger(std::unique_ptr<qublkcmd_ack_t> qcmd_ack,
                           mm::uptrwd<const int> fd_notify);
  ~CmdAcknowledger() override = default;

  CmdAcknowledger(CmdAcknowledger const &) = delete;
  CmdAcknowledger &operator=(CmdAcknowledger const &) = delete;

  CmdAcknowledger(CmdAcknowledger &&) = default;
  CmdAcknowledger &operator=(CmdAcknowledger &&) = default;

  int handle(ublkdrv_cmd_ack cmd) noexcept override;

private:
  std::unique_ptr<qublkcmd_ack_t> qcmd_ack_;
  mm::uptrwd<const int> fd_notify_;
};

} // namespace ublk
