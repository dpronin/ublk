#include "cmd_handler.hpp"

#include <cstddef>

#include <algorithm>
#include <memory>
#include <ranges>

#include <linux/ublkdrv/cmd.h>

#include <gsl/assert>

#include "discard_req.hpp"
#include "flush_req.hpp"
#include "read_req.hpp"
#include "req_handler_not_supp.hpp"
#include "write_req.hpp"

namespace ublk {

CmdHandler::CmdHandler(
    std::span<ublkdrv_celld const> cellds, std::span<std::byte> cells,
    std::shared_ptr<IHandler<int(ublkdrv_cmd_ack) noexcept>> acknowledger,
    std::map<ublkdrv_cmd_op, std::shared_ptr<IUblkReqHandler>> const &maphs)
    : cellds_(cellds), cells_(cells), acknowledger_(std::move(acknowledger)) {
  Ensures(acknowledger_);

  static auto reqh_not_supp{ReqHandlerNotSupp{}};
  static auto const sp_reqh_not_supp{
      std::shared_ptr<IUblkReqHandler>{&reqh_not_supp,
                                       []([[maybe_unused]] auto *p) {}},
  };

  std::ranges::fill(hs_, sp_reqh_not_supp);

  auto hs{std::views::all(hs_) | std::views::take(std::size(hs_) - 1)};
  for (auto const &[op, h] : maphs) {
    Expects(op < std::size(hs));
    hs[op] = h;
  }
}

int CmdHandler::handle(ublkdrv_cmd const &cmd) noexcept {
  auto rq = std::shared_ptr<req>{};

  auto completer{
      [a = acknowledger_](req const &rq) {
        ublkdrv_cmd_ack cmd_ack;
        ublkdrv_cmd_ack_set_id(&cmd_ack, ublkdrv_cmd_get_id(&rq.cmd()));
        ublkdrv_cmd_ack_set_err(&cmd_ack, static_cast<__u16>(rq.err()));
        a->handle(cmd_ack);
      },
  };

  auto const op = ublkdrv_cmd_get_op(&cmd);
  switch (op) {
  case UBLKDRV_CMD_OP_READ:
    rq = read_req::create(cmd, cellds_, cells_, std::move(completer));
    break;
  case UBLKDRV_CMD_OP_WRITE:
    rq = write_req::create(cmd, cellds_, cells_, std::move(completer));
    break;
  case UBLKDRV_CMD_OP_DISCARD:
    rq = discard_req::create(cmd, std::move(completer));
    break;
  case UBLKDRV_CMD_OP_FLUSH:
    rq = flush_req::create(cmd, std::move(completer));
    break;
  default:
    rq = {
        new req{cmd},
        [c = std::move(completer)](req *p) {
          c(*p);
          delete p;
        },
    };
    break;
  }

  auto const hid{std::min(static_cast<size_t>(op), std::size(hs_) - 1)};
  Expects(hs_[hid]);
  return hs_[hid]->handle(std::move(rq));
}

} // namespace ublk
