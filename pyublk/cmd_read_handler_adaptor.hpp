#pragma once

#include <cassert>

#include <format>
#include <memory>
#include <span>
#include <sstream>
#include <string>
#include <utility>

#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include <linux/ublkdrv/celld.h>
#include <linux/ublkdrv/cmd.h>

#include "handler_interface.hpp"
#include "ublk_req_handler_interface.hpp"
#include "utility.hpp"

inline std::ostream &operator<<(std::ostream &out, ublkdrv_cmd_read cmd) {
  auto const &base = *ublk::container_of(
      ublk::container_of(&cmd, &decltype(ublkdrv_cmd::u)::r), &ublkdrv_cmd::u);
  out << std::format(
      "cmd: READ [ id={}, op={}, fl={} fcdn={}, cds_nr={} off={} ]",
      ublkdrv_cmd_get_id(&base), ublkdrv_cmd_get_op(&base),
      ublkdrv_cmd_get_fl(&base), ublkdrv_cmd_read_get_fcdn(&cmd),
      ublkdrv_cmd_read_get_cds_nr(&cmd), ublkdrv_cmd_read_get_offset(&cmd));
  return out;
}

template <>
struct fmt::formatter<ublkdrv_cmd_read> : fmt::formatter<std::string> {
  decltype(auto) format(ublkdrv_cmd_read cmd, format_context &ctx) {
    std::ostringstream oss;
    oss << cmd;
    return fmt::format_to(ctx.out(), "{}", oss.str());
  }
};

namespace ublk {

class CmdReadHandlerAdaptor : public IUblkReqHandler {
public:
  explicit CmdReadHandlerAdaptor(
      std::shared_ptr<
          IHandler<int(ublkdrv_cmd_read, std::span<ublkdrv_celld const>,
                       std::span<std::byte>) noexcept>>
          handler)
      : handler_(std::move(handler)) {
    assert(handler_);
  }
  ~CmdReadHandlerAdaptor() override = default;

  CmdReadHandlerAdaptor(CmdReadHandlerAdaptor const &) = default;
  CmdReadHandlerAdaptor &operator=(CmdReadHandlerAdaptor const &) = default;

  CmdReadHandlerAdaptor(CmdReadHandlerAdaptor &&) = default;
  CmdReadHandlerAdaptor &operator=(CmdReadHandlerAdaptor &&) = default;

  int handle(std::shared_ptr<ublk_req> req) noexcept override {
    assert(UBLKDRV_CMD_OP_READ == ublkdrv_cmd_get_op(&req->cmd()));
    spdlog::debug("process {}", req->cmd().u.r);
    req->set_err(handler_->handle(req->cmd().u.r, req->cellds(), req->cells()));
    return 0;
  }

private:
  std::shared_ptr<IHandler<int(ublkdrv_cmd_read, std::span<ublkdrv_celld const>,
                               std::span<std::byte>) noexcept>>
      handler_;
};

} // namespace ublk
