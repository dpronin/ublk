#pragma once

#include <cassert>

#include <memory>
#include <sstream>
#include <string>
#include <utility>

#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include <linux/ublk/cmd.h>

#include "handler_interface.hpp"
#include "ublk_req_handler_interface.hpp"
#include "utility.hpp"

inline std::ostream &operator<<(std::ostream &out, ublk_cmd_flush cmd) {
  auto const &base = *ublk::container_of(
      ublk::container_of(&cmd, &decltype(ublk_cmd::u)::f), &ublk_cmd::u);
  out << fmt::format("cmd: FLUSH [ id={}, op={}, fl={} ]",
                     ublk_cmd_get_id(&base), ublk_cmd_get_op(&base),
                     ublk_cmd_get_fl(&base));
  return out;
}

template <>
struct fmt::formatter<ublk_cmd_flush> : fmt::formatter<std::string> {
  decltype(auto) format(ublk_cmd_flush cmd, format_context &ctx) {
    std::ostringstream oss;
    oss << cmd;
    return format_to(ctx.out(), "{}", oss.str());
  }
};

namespace ublk {

class CmdFlushHandlerAdaptor : public IUblkReqHandler {
public:
  explicit CmdFlushHandlerAdaptor(
      std::shared_ptr<IHandler<int(ublk_cmd_flush) noexcept>> handler)
      : handler_(std::move(handler)) {
    assert(handler_);
  }
  ~CmdFlushHandlerAdaptor() override = default;

  CmdFlushHandlerAdaptor(CmdFlushHandlerAdaptor const &) = default;
  CmdFlushHandlerAdaptor &operator=(CmdFlushHandlerAdaptor const &) = default;

  CmdFlushHandlerAdaptor(CmdFlushHandlerAdaptor &&) = default;
  CmdFlushHandlerAdaptor &operator=(CmdFlushHandlerAdaptor &&) = default;

  int handle(std::shared_ptr<ublk_req> req) noexcept override {
    assert(UBLK_CMD_OP_FLUSH == ublk_cmd_get_op(&req->cmd()));
    spdlog::debug("process {}", req->cmd().u.f);
    req->set_err(handler_->handle(req->cmd().u.f));
    return 0;
  }

private:
  std::shared_ptr<IHandler<int(ublk_cmd_flush) noexcept>> handler_;
};

} // namespace ublk
