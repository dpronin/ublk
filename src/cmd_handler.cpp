#include "cmd_handler.hpp"

#include <linux/types.h>

#include <linux/ublk/cmd.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include <algorithm>
#include <ostream>
#include <ranges>
#include <sstream>
#include <utility>

#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include "cmd_handler_not_supp.hpp"

namespace cfq {

CmdHandler::CmdHandler(
    std::map<ublk_cmd_op, std::shared_ptr<ICmdHandler<const ublk_cmd>>> maphs,
    std::shared_ptr<ICmdHandler<const ublk_cmd_ack>> acknowledger)
    : acknowledger_(std::move(acknowledger)) {

  assert(acknowledger_);

  /* clang-format off */
  static auto cmdh_not_supp = CmdHandlerNotSupp{};
  static auto const sp_cmdh_not_supp =
      std::shared_ptr<ICmdHandler<const ublk_cmd>>{&cmdh_not_supp, [](auto *p) {}};
  /* clang-format on */

  std::ranges::fill(hs_, sp_cmdh_not_supp);

  for (auto &[op, h] : maphs) {
    assert(op < std::size(hs_));
    hs_[op] = std::move(h);
  }
}

int CmdHandler::handle(ublk_cmd const &cmd) noexcept {
  auto const op = ublk_cmd_get_op(&cmd);
  auto const hid = std::min(static_cast<size_t>(op), std::size(hs_) - 1);

  assert(hs_[hid]);

  auto const r = hs_[hid]->handle(cmd);

  ublk_cmd_ack cmd_ack;
  ublk_cmd_ack_set_id(&cmd_ack, ublk_cmd_get_id(&cmd));
  ublk_cmd_ack_set_err(&cmd_ack, static_cast<__u16>(r));

  return acknowledger_->handle(cmd_ack);
}

} // namespace cfq
