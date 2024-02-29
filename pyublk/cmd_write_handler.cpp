#include "cmd_write_handler.hpp"

#include <cassert>
#include <cstddef>

#include <memory>
#include <utility>

#include <linux/ublkdrv/cmd.h>

#include "for_each_celld.hpp"
#include "write_query.hpp"

namespace ublk {

CmdWriteHandler::CmdWriteHandler(std::shared_ptr<IWriteHandler> writer)
    : writer_(std::move(writer)) {
  assert(writer_);
}

int CmdWriteHandler::handle(std::shared_ptr<write_req> req) noexcept {
  assert(req);
  auto *p_req = req.get();
  return for_each_celld(
      ublkdrv_cmd_write_get_offset(&req->cmd()),
      ublkdrv_cmd_write_get_fcdn(&p_req->cmd()),
      ublkdrv_cmd_write_get_cds_nr(&p_req->cmd()), p_req->cellds(),
      p_req->cells(),
      [w = writer_.get(), req = std::move(req)](
          uint64_t offset, std::span<std::byte const> buf) mutable {
        auto completer = [req](write_query const &wq) {
          if (wq.err())
            req->set_err(wq.err());
        };
        auto wq = write_query::create(buf, offset, std::move(completer));
        if (auto const res{w->submit(std::move(wq))}) [[unlikely]] {
          return res;
        }
        return 0;
      });
}

} // namespace ublk
