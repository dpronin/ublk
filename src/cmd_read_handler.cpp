#include "cmd_read_handler.hpp"

#include <cassert>
#include <cstdint>
#include <cstdlib>

#include <utility>

#include "for_each_celld.hpp"

namespace ublk {

CmdReadHandler::CmdReadHandler(std::shared_ptr<IReadHandler> reader)
    : reader_(std::move(reader)) {

  assert(reader_);
}

int CmdReadHandler::handle(ublk_cmd_read cmd,
                           std::span<ublk_celld const> cellds,
                           std::span<std::byte> cells) noexcept {

  return for_each_celld(ublk_cmd_read_get_fcdn(&cmd),
                        ublk_cmd_read_get_cds_nr(&cmd), cellds, cells,
                        [this, offset = ublk_cmd_read_get_offset(&cmd)](
                            std::span<std::byte> buf) mutable {
                          while (!buf.empty()) {
                            if (auto const res = reader_->handle(buf, offset);
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
