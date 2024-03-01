#pragma once

#include <cassert>

#include <format>
#include <memory>
#include <sstream>
#include <string>
#include <utility>

#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include <linux/ublkdrv/cmd.h>

#include "discard_req.hpp"
#include "handler_interface.hpp"
#include "ublk_req_handler_interface.hpp"
#include "utility.hpp"

inline std::ostream &operator<<(std::ostream &out, ublkdrv_cmd_discard cmd) {
  auto const &base = *ublk::container_of(
      ublk::container_of(&cmd, &decltype(ublkdrv_cmd::u)::d), &ublkdrv_cmd::u);
  out << std::format("cmd: DISCARD [ id={}, op={}, fl={} off={} sz={} ]",
                     ublkdrv_cmd_get_id(&base), ublkdrv_cmd_get_op(&base),
                     ublkdrv_cmd_get_fl(&base),
                     ublkdrv_cmd_discard_get_offset(&cmd),
                     ublkdrv_cmd_discard_get_sz(&cmd));
  return out;
}

template <>
struct fmt::formatter<ublkdrv_cmd_discard> : fmt::formatter<std::string> {
  decltype(auto) format(ublkdrv_cmd_discard cmd, format_context &ctx) {
    std::ostringstream oss;
    oss << cmd;
    return fmt::format_to(ctx.out(), "{}", oss.str());
  }
};

namespace ublk {

class CmdDiscardHandlerAdaptor : public IUblkReqHandler {
public:
  explicit CmdDiscardHandlerAdaptor(
      std::shared_ptr<IHandler<int(std::shared_ptr<discard_req>) noexcept>>
          handler)
      : handler_(std::move(handler)) {
    assert(handler_);
  }
  ~CmdDiscardHandlerAdaptor() override = default;

  CmdDiscardHandlerAdaptor(CmdDiscardHandlerAdaptor const &) = default;
  CmdDiscardHandlerAdaptor &
  operator=(CmdDiscardHandlerAdaptor const &) = default;

  CmdDiscardHandlerAdaptor(CmdDiscardHandlerAdaptor &&) = default;
  CmdDiscardHandlerAdaptor &operator=(CmdDiscardHandlerAdaptor &&) = default;

  int handle(std::shared_ptr<req> rq) noexcept override {
    auto drq = std::static_pointer_cast<discard_req>(std::move(rq));
    spdlog::debug("process {}", drq->cmd());
    handler_->handle(std::move(drq));
    return 0;
  }

private:
  std::shared_ptr<IHandler<int(std::shared_ptr<discard_req>) noexcept>>
      handler_;
};

} // namespace ublk
