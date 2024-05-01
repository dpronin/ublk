#include "cmd_read_handler.hpp"

#include <cassert>
#include <cstddef>

#include <memory>
#include <utility>

#include <linux/ublkdrv/cmd.h>

#include "for_each_celld.hpp"
#include "read_query.hpp"

namespace ublk {

CmdReadHandler::CmdReadHandler(std::shared_ptr<IRDQSubmitter> reader)
    : reader_(std::move(reader)) {
  assert(reader_);
}

int CmdReadHandler::handle(std::shared_ptr<read_req> req) noexcept {
  assert(req);
  auto *p_req = req.get();
  return for_each_celld(
      ublkdrv_cmd_read_get_offset(&p_req->cmd()),
      ublkdrv_cmd_read_get_fcdn(&p_req->cmd()),
      ublkdrv_cmd_read_get_cds_nr(&p_req->cmd()), p_req->cellds(),
      p_req->cells(),
      [r = reader_.get(), req = std::move(req)](uint64_t offset,
                                                std::span<std::byte> buf) {
        auto completer = [req](read_query const &rq) {
          if (rq.err())
            req->set_err(rq.err());
        };
        auto rq = read_query::create(buf, offset, std::move(completer));
        if (auto const res{r->submit(std::move(rq))}) [[unlikely]] {
          return res;
        }
        return 0;
      });
}

} // namespace ublk
