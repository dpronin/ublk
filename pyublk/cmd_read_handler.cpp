#include "cmd_read_handler.hpp"

#include <cassert>
#include <cstddef>

#include <utility>

#include <linux/ublkdrv/cmd.h>

#include "for_each_celld.hpp"

namespace ublk {

CmdReadHandler::CmdReadHandler(std::shared_ptr<IReadHandler> reader)
    : reader_(std::move(reader)) {
  assert(reader_);
}

int CmdReadHandler::handle(ublkdrv_cmd_read cmd,
                           std::span<ublkdrv_celld const> cellds,
                           std::span<std::byte> cells) noexcept {
  return for_each_celld(
      ublkdrv_cmd_read_get_offset(&cmd), ublkdrv_cmd_read_get_fcdn(&cmd),
      ublkdrv_cmd_read_get_cds_nr(&cmd), cellds, cells,
      [r = reader_.get()](uint64_t offset, std::span<std::byte> buf) {
        while (!buf.empty()) {
          if (auto const res = r->handle(buf, offset); res > 0) [[likely]] {
            buf = buf.subspan(
                std::min(static_cast<std::size_t>(res), buf.size()));
          } else if (res < 0) [[unlikely]] {
            return -static_cast<int>(res);
          }
        }
        return 0;
      });
}

} // namespace ublk
