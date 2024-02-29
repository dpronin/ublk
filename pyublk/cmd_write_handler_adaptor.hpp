#pragma once

#include <cassert>

#include <format>
#include <memory>
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
#include "write_req.hpp"

inline std::ostream &operator<<(std::ostream &out, ublkdrv_cmd_write cmd) {
  auto const &base = *ublk::container_of(
      ublk::container_of(&cmd, &decltype(ublkdrv_cmd::u)::w), &ublkdrv_cmd::u);
  out << std::format(
      "cmd: WRITE [ id={}, op={}, fl={} fcdn={}, cds_nr={} off={} ]",
      ublkdrv_cmd_get_id(&base), ublkdrv_cmd_get_op(&base),
      ublkdrv_cmd_get_fl(&base), ublkdrv_cmd_write_get_fcdn(&cmd),
      ublkdrv_cmd_write_get_cds_nr(&cmd), ublkdrv_cmd_write_get_offset(&cmd));
  return out;
}

template <>
struct fmt::formatter<ublkdrv_cmd_write> : fmt::formatter<std::string> {
  decltype(auto) format(ublkdrv_cmd_write cmd, format_context &ctx) {
    std::ostringstream oss;
    oss << cmd;
    return fmt::format_to(ctx.out(), "{}", oss.str());
  }
};

namespace ublk {

class CmdWriteHandlerAdaptor : public IUblkReqHandler {
public:
  explicit CmdWriteHandlerAdaptor(
      std::shared_ptr<IHandler<int(std::shared_ptr<write_req>) noexcept>>
          handler)
      : handler_(std::move(handler)) {
    assert(handler_);
  }
  ~CmdWriteHandlerAdaptor() override = default;

  CmdWriteHandlerAdaptor(CmdWriteHandlerAdaptor const &) = default;
  CmdWriteHandlerAdaptor &operator=(CmdWriteHandlerAdaptor const &) = default;

  CmdWriteHandlerAdaptor(CmdWriteHandlerAdaptor &&) = default;
  CmdWriteHandlerAdaptor &operator=(CmdWriteHandlerAdaptor &&) = default;

  int handle(std::shared_ptr<req> rq) noexcept override {
    auto wrq = write_req::create(std::move(rq));
    spdlog::debug("process {}", wrq->cmd());
    handler_->handle(std::move(wrq));
    return 0;
  }

private:
  std::shared_ptr<IHandler<int(std::shared_ptr<write_req>) noexcept>> handler_;
};

} // namespace ublk
