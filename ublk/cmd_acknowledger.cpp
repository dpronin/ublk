#include "cmd_acknowledger.hpp"

#include <unistd.h>

#include <cstdint>
#include <cstdlib>

#include <format>
#include <ostream>
#include <sstream>
#include <thread>
#include <utility>

#include <fmt/format.h>
#include <gsl/assert>
#include <spdlog/spdlog.h>

inline std::ostream &operator<<(std::ostream &out, ublkdrv_cmd_ack const &cmd) {
  out << std::format("cmd-ack: [ id={}, err={} ]", ublkdrv_cmd_ack_get_id(&cmd),
                     ublkdrv_cmd_ack_get_err(&cmd));
  return out;
}

template <>
struct fmt::formatter<ublkdrv_cmd_ack> : fmt::formatter<std::string> {
  auto format(ublkdrv_cmd_ack const &cmd, format_context &ctx) const {

    std::ostringstream oss;
    oss << cmd;
    return fmt::formatter<std::string>::format(oss.str(), ctx);
  }
};

namespace ublk {

CmdAcknowledger::CmdAcknowledger(std::unique_ptr<qublkcmd_ack_t> qcmd_ack,
                                 mm::uptrwd<const int> fd_notify)
    : qcmd_ack_(std::move(qcmd_ack)), fd_notify_(std::move(fd_notify)) {
  Ensures(qcmd_ack_);
  Ensures(fd_notify_);
}

int CmdAcknowledger::handle(ublkdrv_cmd_ack cmd) noexcept {
  spdlog::debug("acknowledging {}", cmd);

  while (!qcmd_ack_->push(cmd))
    std::this_thread::yield();

  if (uint32_t new_cmds = 1;
      4 != write(*fd_notify_, &new_cmds, sizeof(new_cmds))) [[unlikely]] {

    return EXIT_FAILURE;
  }

  return 0;
}

} // namespace ublk
