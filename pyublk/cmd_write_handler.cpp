#include "cmd_write_handler.hpp"

#include <cassert>
#include <cstddef>

#include <utility>

#include <linux/ublkdrv/cmd.h>

#include "for_each_celld.hpp"

namespace ublk {

CmdWriteHandler::CmdWriteHandler(std::shared_ptr<IWriteHandler> writer)
    : writer_(std::move(writer)) {
  assert(writer_);
}

int CmdWriteHandler::handle(ublkdrv_cmd_write cmd,
                            std::span<ublkdrv_celld const> cellds,
                            std::span<std::byte const> cells) noexcept {
  return for_each_celld(
      ublkdrv_cmd_write_get_offset(&cmd), ublkdrv_cmd_write_get_fcdn(&cmd),
      ublkdrv_cmd_write_get_cds_nr(&cmd), cellds, cells,
      [w = writer_.get()](uint64_t offset, std::span<std::byte const> buf) {
        while (!buf.empty()) {
          if (auto const res = w->handle(buf, offset); res > 0) [[likely]] {
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
