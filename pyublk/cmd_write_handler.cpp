#include "cmd_write_handler.hpp"

#include <cassert>

#include <utility>

#include <linux/ublk/cmd.h>

#include "for_each_celld.hpp"

namespace ublk {

CmdWriteHandler::CmdWriteHandler(std::shared_ptr<IWriteHandler> writer)
    : writer_(std::move(writer)) {

  assert(writer_);
}

int CmdWriteHandler::handle(ublk_cmd_write cmd,
                            std::span<ublk_celld const> cellds,
                            std::span<std::byte const> cells) noexcept {

  return for_each_celld(ublk_cmd_write_get_fcdn(&cmd),
                        ublk_cmd_write_get_cds_nr(&cmd), cellds, cells,
                        [this, offset = ublk_cmd_write_get_offset(&cmd)](
                            std::span<std::byte const> buf) mutable {
                          while (!buf.empty()) {
                            if (auto const res = writer_->handle(buf, offset);
                                res > 0) [[likely]] {

                              buf = buf.subspan(res);
                              offset += res;
                            } else if (res < 0) [[unlikely]] {
                              return -static_cast<int>(res);
                            }
                          }
                          return 0;
                        });
  return 0;
}

} // namespace ublk
