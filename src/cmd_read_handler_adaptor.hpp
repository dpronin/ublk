#pragma once

#include <cassert>

#include <memory>
#include <sstream>
#include <string>
#include <utility>

#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include <linux/ublk/cmd.h>

#include "cmd_handler_interface.hpp"
#include "utility.hpp"

inline std::ostream &operator<<(std::ostream &out,
                                struct ublk_cmd_read const &cmd) {
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
  decltype(auto) format(ublk_cmd_read const &cmd, format_context &ctx) {
    std::ostringstream oss;
    oss << cmd;
    return format_to(ctx.out(), "{}", oss.str());
  }
};

namespace cfq {

class CmdReadHandlerAdaptor : public ICmdHandler<const ublk_cmd> {
public:
  explicit CmdReadHandlerAdaptor(
      std::shared_ptr<ICmdHandler<const ublk_cmd_read>> handler)
      : handler_(std::move(handler)) {
    assert(handler_);
  }
  ~CmdReadHandlerAdaptor() override = default;

  CmdReadHandlerAdaptor(CmdReadHandlerAdaptor const &) = default;
  CmdReadHandlerAdaptor &operator=(CmdReadHandlerAdaptor const &) = default;

  CmdReadHandlerAdaptor(CmdReadHandlerAdaptor &&) = default;
  CmdReadHandlerAdaptor &operator=(CmdReadHandlerAdaptor &&) = default;

  int handle(ublk_cmd const &cmd) noexcept override {
    assert(UBLK_CMD_OP_READ == ublk_cmd_get_op(&cmd));
    spdlog::debug("process {}", cmd.u.r);
    return handler_->handle(cmd.u.r);
  }

private:
  std::shared_ptr<ICmdHandler<const ublk_cmd_read>> handler_;
};

} // namespace cfq
