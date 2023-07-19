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
#include "utility.hpp"

inline std::ostream &operator<<(std::ostream &out, ublk_cmd_write cmd) {
  auto const &base = *cfq::container_of(
      cfq::container_of(&cmd, &decltype(ublk_cmd::u)::w), &ublk_cmd::u);
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

namespace cfq {

class CmdWriteHandlerAdaptor : public IHandler<int(ublk_cmd) noexcept> {
public:
  explicit CmdWriteHandlerAdaptor(
      std::shared_ptr<IHandler<int(ublk_cmd_write) noexcept>> handler)
      : handler_(std::move(handler)) {
    assert(handler_);
  }
  ~CmdWriteHandlerAdaptor() override = default;

  CmdWriteHandlerAdaptor(CmdWriteHandlerAdaptor const &) = default;
  CmdWriteHandlerAdaptor &operator=(CmdWriteHandlerAdaptor const &) = default;

  CmdWriteHandlerAdaptor(CmdWriteHandlerAdaptor &&) = default;
  CmdWriteHandlerAdaptor &operator=(CmdWriteHandlerAdaptor &&) = default;

  int handle(ublk_cmd cmd) noexcept override {
    assert(UBLK_CMD_OP_WRITE == ublk_cmd_get_op(&cmd));
    spdlog::debug("process {}", cmd.u.w);
    return handler_->handle(cmd.u.w);
  }

private:
  std::shared_ptr<IHandler<int(ublk_cmd_write) noexcept>> handler_;
};

} // namespace cfq
