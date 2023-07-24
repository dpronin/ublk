#pragma once

#include <cassert>

#include <memory>
#include <span>
#include <sstream>
#include <string>
#include <utility>

#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include <linux/ublk/celld.h>
#include <linux/ublk/cmd.h>

#include "handler_interface.hpp"
#include "ublk_req_handler_interface.hpp"
#include "utility.hpp"

inline std::ostream &operator<<(std::ostream &out, ublk_cmd_read cmd) {
  auto const &base = *ublk::container_of(
      ublk::container_of(&cmd, &decltype(ublk_cmd::u)::r), &ublk_cmd::u);
  out << fmt::format(
      "cmd: READ [ id={}, op={}, fl={} fcdn={}, cds_nr={} off={} ]",
      ublk_cmd_get_id(&base), ublk_cmd_get_op(&base), ublk_cmd_get_fl(&base),
      ublk_cmd_read_get_fcdn(&cmd), ublk_cmd_read_get_cds_nr(&cmd),
      ublk_cmd_read_get_offset(&cmd));
  return out;
}

template <> struct fmt::formatter<ublk_cmd_read> : fmt::formatter<std::string> {
  decltype(auto) format(ublk_cmd_read cmd, format_context &ctx) {
    std::ostringstream oss;
    oss << cmd;
    return format_to(ctx.out(), "{}", oss.str());
  }
};

namespace ublk {

class CmdReadHandlerAdaptor : public IUblkReqHandler {
public:
  explicit CmdReadHandlerAdaptor(
      std::shared_ptr<IHandler<int(ublk_cmd_read, std::span<ublk_celld const>,
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
    assert(UBLK_CMD_OP_READ == ublk_cmd_get_op(&req->cmd()));
    spdlog::debug("process {}", req->cmd().u.r);
    req->set_err(handler_->handle(req->cmd().u.r, req->cellds(), req->cells()));
    return 0;
  }

private:
  std::shared_ptr<IHandler<int(ublk_cmd_read, std::span<ublk_celld const>,
                               std::span<std::byte>) noexcept>>
      handler_;
};

} // namespace ublk
