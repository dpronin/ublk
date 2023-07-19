#include "cmd_read_handler.hpp"

#include <cassert>
#include <cstdint>
#include <cstdlib>

#include <utility>

namespace cfq {

CmdReadHandler::CmdReadHandler(std::shared_ptr<ublk_cellc const> cellc,
                               std::span<std::byte> cells,
                               std::shared_ptr<IReadHandler> reader)
    : cellc_(std::move(cellc)), cells_(cells), reader_(std::move(reader)) {

  assert(cellc_);
  assert(reader_);
}

int CmdReadHandler::handle(ublk_cmd_read cmd) noexcept {
  auto celldn = ublk_cmd_read_get_fcdn(&cmd);
  if (auto cellds_left = ublk_cmd_read_get_cds_nr(&cmd);
      cellds_left > 0 && celldn < cellc_->cellds_len) [[likely]] {
    auto offset = ublk_cmd_read_get_offset(&cmd);
    for (auto const *celld = &cellc_->cellds[celldn];
         cellds_left > 0 && celldn < cellc_->cellds_len; celldn = celld->ncelld,
                    celld = &cellc_->cellds[celldn], --cellds_left) {

      if (!(celld->offset + celld->data_sz <= cells_.size())) [[unlikely]] {
        return EINVAL;
      }

      /* clang-format off */
      for (decltype(celld->offset) cell_offset = 0; cell_offset < celld->data_sz; ) {
        if (auto const res = reader_->handle({&cells_[celld->offset + cell_offset],
                                        celld->data_sz - cell_offset}, offset);
            res > 0) [[likely]] {

          cell_offset += res;
          offset += res;
        } else if (res < 0) {
          return -res;
        }
      }
      /* clang-format on */
    }
  }

  return 0;
}

} // namespace cfq
