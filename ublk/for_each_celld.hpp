#pragma once

#include <cerrno>

#include <span>

#include <gsl/assert>

#include <linux/ublkdrv/celld.h>

#include "utils/concepts.hpp"

namespace ublk {

constexpr auto for_each_celld = []<is_byte T>(
                                    std::unsigned_integral auto offset,
                                    std::unsigned_integral auto celldn,
                                    std::unsigned_integral auto cellds_nr,
                                    std::span<ublkdrv_celld const> cellds,
                                    std::span<T> cells, auto &&f,
                                    auto &&...args) {
  if (auto cellds_left{cellds_nr}; cellds_left > 0 && celldn < cellds.size())
      [[likely]] {
    for (auto const *celld{&cellds[celldn]};
         cellds_left > 0 && celldn < cellds.size();
         celldn = celld->ncelld, celld = &cellds[celldn], --cellds_left) {

      if (!(celld->offset + celld->data_sz <= cells.size())) [[unlikely]]
        return EINVAL;

      Expects(!(celld->offset + celld->data_sz > cells.size()));

      auto const buf{cells.subspan(celld->offset, celld->data_sz)};

      if (auto const r{f(offset, buf, std::forward<decltype(args)>(args)...)})
          [[unlikely]] {
        return r;
      }

      offset += buf.size();
    }
  }
  return 0;
};

} // namespace ublk
