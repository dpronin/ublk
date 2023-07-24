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

inline std::ostream &operator<<(std::ostream &out, ublk_cmd_write cmd) {
  auto const &base = *ublk::container_of(
      ublk::container_of(&cmd, &decltype(ublk_cmd::u)::w), &ublk_cmd::u);
  out << fmt::format(
      "cmd: WRITE [ id={}, op={}, fl={} fcdn={}, cds_nr={} off={} ]",
      ublk_cmd_get_id(&base), ublk_cmd_get_op(&base), ublk_cmd_get_fl(&base),
      ublk_cmd_write_get_fcdn(&cmd), ublk_cmd_write_get_cds_nr(&cmd),
      ublk_cmd_write_get_offset(&cmd));
  return out;
}

template <>
struct fmt::formatter<ublk_cmd_write> : fmt::formatter<std::string> {
  decltype(auto) format(ublk_cmd_write cmd, format_context &ctx) {
    std::ostringstream oss;
    oss << cmd;
    return format_to(ctx.out(), "{}", oss.str());
  }
};

namespace ublk {

class CmdWriteHandlerAdaptor : public IUblkReqHandler {
public:
  explicit CmdWriteHandlerAdaptor(
      std::shared_ptr<IHandler<int(ublk_cmd_write, ublk_cellc const &,
                                   std::span<std::byte const>) noexcept>>
          handler)
      : handler_(std::move(handler)) {
    assert(handler_);
  }
  ~CmdWriteHandlerAdaptor() override = default;

  CmdWriteHandlerAdaptor(CmdWriteHandlerAdaptor const &) = default;
  CmdWriteHandlerAdaptor &operator=(CmdWriteHandlerAdaptor const &) = default;

  CmdWriteHandlerAdaptor(CmdWriteHandlerAdaptor &&) = default;
  CmdWriteHandlerAdaptor &operator=(CmdWriteHandlerAdaptor &&) = default;

  int handle(std::shared_ptr<ublk_req> req) noexcept override {
    assert(UBLK_CMD_OP_WRITE == ublk_cmd_get_op(&req->cmd()));
    spdlog::debug("process {}", req->cmd().u.w);
    req->set_err(handler_->handle(req->cmd().u.w, req->cellc(), req->cells()));
    return 0;
  }

private:
  std::shared_ptr<IHandler<int(ublk_cmd_write, ublk_cellc const &,
                               std::span<std::byte const>) noexcept>>
      handler_;
};

} // namespace ublk
