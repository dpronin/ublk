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

inline std::ostream &operator<<(std::ostream &out, ublk_cmd_discard cmd) {
  auto const &base = *cfq::container_of(
      cfq::container_of(&cmd, &decltype(ublk_cmd::u)::d), &ublk_cmd::u);
  out << fmt::format("cmd: DISCARD [ id={}, op={}, fl={} off={} sz={} ]",
                     ublk_cmd_get_id(&base), ublk_cmd_get_op(&base),
                     ublk_cmd_get_fl(&base), ublk_cmd_discard_get_offset(&cmd),
                     ublk_cmd_discard_get_sz(&cmd));
  return out;
}

template <>
struct fmt::formatter<ublk_cmd_discard> : fmt::formatter<std::string> {
  decltype(auto) format(ublk_cmd_discard cmd, format_context &ctx) {
    std::ostringstream oss;
    oss << cmd;
    return format_to(ctx.out(), "{}", oss.str());
  }
};

namespace cfq {

class CmdDiscardHandlerAdaptor : public IHandler<int(ublk_cmd) noexcept> {
public:
  explicit CmdDiscardHandlerAdaptor(
      std::shared_ptr<IHandler<int(ublk_cmd_discard) noexcept>> handler)
      : handler_(std::move(handler)) {
    assert(handler_);
  }
  ~CmdDiscardHandlerAdaptor() override = default;

  CmdDiscardHandlerAdaptor(CmdDiscardHandlerAdaptor const &) = default;
  CmdDiscardHandlerAdaptor &
  operator=(CmdDiscardHandlerAdaptor const &) = default;

  CmdDiscardHandlerAdaptor(CmdDiscardHandlerAdaptor &&) = default;
  CmdDiscardHandlerAdaptor &operator=(CmdDiscardHandlerAdaptor &&) = default;

  int handle(ublk_cmd cmd) noexcept override {
    assert(UBLK_CMD_OP_DISCARD == ublk_cmd_get_op(&cmd));
    spdlog::debug("process {}", cmd.u.d);
    return handler_->handle(cmd.u.d);
  }

private:
  std::shared_ptr<IHandler<int(ublk_cmd_discard) noexcept>> handler_;
};

} // namespace cfq
