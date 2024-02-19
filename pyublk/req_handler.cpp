#include "req_handler.hpp"

#include <cassert>
#include <cstddef>

#include <algorithm>
#include <memory>
#include <ranges>
#include <utility>

#include "req_handler_not_supp.hpp"

namespace ublk {

ReqHandler::ReqHandler(
    std::map<ublkdrv_cmd_op, std::shared_ptr<IUblkReqHandler>> const &maphs) {

  /* clang-format off */
  static auto reqh_not_supp = ReqHandlerNotSupp{};
  static auto const sp_reqh_not_supp =
      std::shared_ptr<IUblkReqHandler>{&reqh_not_supp, []([[maybe_unused]] auto *p) {}};
  /* clang-format on */

  std::ranges::fill(hs_, sp_reqh_not_supp);

  auto hs = hs_ | std::views::take(std::size(hs_) - 1);
  for (auto const &[op, h] : maphs) {
    assert(op < std::size(hs));
    hs[op] = h;
  }
}

int ReqHandler::handle(std::shared_ptr<ublk_req> req) noexcept {
  auto const op = ublkdrv_cmd_get_op(&req->cmd());
  auto const hid = std::min(static_cast<size_t>(op), std::size(hs_) - 1);
  assert(hs_[hid]);
  return hs_[hid]->handle(std::move(req));
}

} // namespace ublk
