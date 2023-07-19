#include "cmd_write_handler.hpp"

#include <cassert>
#include <cstdint>
#include <cstdlib>

#include <utility>

namespace cfq {

CmdWriteHandler::CmdWriteHandler(std::shared_ptr<ublk_cellc const> cellc,
                                 std::span<std::byte const> cells,
                                 std::shared_ptr<IWriteHandler> writer)
    : cellc_(std::move(cellc)), cells_(cells), writer_(std::move(writer)) {

  assert(cellc_);
  assert(writer_);
}

int CmdWriteHandler::handle(ublk_cmd_write cmd) noexcept {
  auto celldn = ublk_cmd_write_get_fcdn(&cmd);
  auto cellds_left = ublk_cmd_write_get_cds_nr(&cmd);
  if (cellds_left > 0 && celldn < cellc_->cellds_len) [[likely]] {
    auto offset = ublk_cmd_write_get_offset(&cmd);
    for (auto const *celld = &cellc_->cellds[celldn];
         cellds_left > 0 && celldn < cellc_->cellds_len; celldn = celld->ncelld,
                    celld = &cellc_->cellds[celldn], --cellds_left) {

      if (!(celld->offset + celld->data_sz <= cells_.size())) [[unlikely]] {
        return EINVAL;
      }

      /* clang-format off */
      for (decltype(celld->offset) cell_offset = 0; cell_offset < celld->data_sz;) {
        if (auto const res = writer_->handle({&cells_[celld->offset + cell_offset], celld->data_sz - cell_offset}, offset); res >= 0) [[likely]] {
          cell_offset += res;
          offset += res;
        } else {
          return -res;
        }
      }
      /* clang-format on */
    }
  }

  return 0;
}

} // namespace cfq
