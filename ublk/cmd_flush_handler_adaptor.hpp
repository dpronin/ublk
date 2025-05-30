#pragma once

#include <format>
#include <memory>
#include <sstream>
#include <string>
#include <utility>

#include <fmt/format.h>
#include <gsl/assert>
#include <spdlog/spdlog.h>

#include <linux/ublkdrv/cmd.h>

#include "utils/utility.hpp"

#include "flush_req.hpp"
#include "handler_interface.hpp"
#include "ublk_req_handler_interface.hpp"

inline std::ostream &operator<<(std::ostream &out, ublkdrv_cmd_flush cmd) {
  auto const &base = *ublk::container_of(
      ublk::container_of(&cmd, &decltype(ublkdrv_cmd::u)::f), &ublkdrv_cmd::u);
  out << std::format("cmd: FLUSH [ id={}, op={}, fl={} ]",
                     ublkdrv_cmd_get_id(&base), ublkdrv_cmd_get_op(&base),
                     ublkdrv_cmd_get_fl(&base));
  return out;
}

template <>
struct fmt::formatter<ublkdrv_cmd_flush> : fmt::formatter<std::string> {
  auto format(ublkdrv_cmd_flush cmd, format_context &ctx) const {
    std::ostringstream oss;
    oss << cmd;
    return fmt::formatter<std::string>::format(oss.str(), ctx);
  }
};

namespace ublk {

class CmdFlushHandlerAdaptor : public IUblkReqHandler {
public:
  explicit CmdFlushHandlerAdaptor(
      std::shared_ptr<IHandler<int(std::shared_ptr<flush_req>) noexcept>>
          handler)
      : handler_(std::move(handler)) {
    Ensures(handler_);
  }
  ~CmdFlushHandlerAdaptor() override = default;

  CmdFlushHandlerAdaptor(CmdFlushHandlerAdaptor const &) = default;
  CmdFlushHandlerAdaptor &operator=(CmdFlushHandlerAdaptor const &) = default;

  CmdFlushHandlerAdaptor(CmdFlushHandlerAdaptor &&) = default;
  CmdFlushHandlerAdaptor &operator=(CmdFlushHandlerAdaptor &&) = default;

  int handle(std::shared_ptr<req> rq) noexcept override {
    auto frq = std::static_pointer_cast<flush_req>(std::move(rq));
    spdlog::debug("process {}", frq->cmd());
    handler_->handle(std::move(frq));
    return 0;
  }

private:
  std::shared_ptr<IHandler<int(std::shared_ptr<flush_req>) noexcept>> handler_;
};

} // namespace ublk
