#pragma once

#include <cassert>

#include <memory>
#include <sstream>
#include <string>
#include <utility>

#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include <linux/ublk/cellc.h>
#include <linux/ublk/cmd.h>

#include "handler_interface.hpp"
#include "ublk_req.hpp"
#include "ublk_req_handler_interface.hpp"
#include "utility.hpp"

inline std::ostream &operator<<(std::ostream &out, ublk_cmd_read cmd) {
  auto const &base = *cfq::container_of(
      cfq::container_of(&cmd, &decltype(ublk_cmd::u)::r), &ublk_cmd::u);
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

namespace cfq {

class CmdReadHandlerAdaptor : public IUblkReqHandler {
public:
  explicit CmdReadHandlerAdaptor(
      std::shared_ptr<IHandler<int(ublk_cmd_read, ublk_cellc const &,
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
    handler_->handle(req->cmd().u.r, req->cellc(), req->cells());
    return 0;
  }

private:
  std::shared_ptr<IHandler<int(ublk_cmd_read, ublk_cellc const &,
                               std::span<std::byte>) noexcept>>
      handler_;
};

} // namespace cfq
