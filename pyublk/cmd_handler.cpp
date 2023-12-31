#include "cmd_handler.hpp"

#include <linux/types.h>

#include <cassert>

#include <memory>
#include <utility>

#include "cache_line_aligned_allocator.hpp"
#include "ublk_req.hpp"
#include "ublk_req_handler_interface.hpp"

namespace ublk {

CmdHandler::CmdHandler(
    std::shared_ptr<IUblkReqHandler> handler,
    std::span<ublk_celld const> cellds, std::span<std::byte> cells,
    std::shared_ptr<IHandler<int(ublk_cmd_ack) noexcept>> acknowledger)
    : handler_(std::move(handler)), cellds_(cellds), cells_(cells),
      acknowledger_(std::move(acknowledger)) {

  assert(handler_);
  assert(acknowledger_);
}

int CmdHandler::handle(ublk_cmd cmd) noexcept {
  auto req = std::allocate_shared<ublk_req>(
      cache_line_aligned_allocator<ublk_req>{}, cmd, cellds_, cells_,
      [a = acknowledger_](ublk_cmd const &cmd, int err) {
        ublk_cmd_ack cmd_ack;
        ublk_cmd_ack_set_id(&cmd_ack, ublk_cmd_get_id(&cmd));
        ublk_cmd_ack_set_err(&cmd_ack, static_cast<__u16>(err));
        a->handle(cmd_ack);
      });
  return handler_->handle(std::move(req));
}

} // namespace ublk
