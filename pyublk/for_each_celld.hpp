#pragma once

#include <cerrno>

#include <concepts>
#include <functional>
#include <span>

#include <linux/ublk/celld.h>

namespace ublk {

constexpr auto for_each_celld = []<typename T /*std::same_as<std::byte> T*/>(
                                    std::integral auto celldn,
                                    std::integral auto cellds_nr,
                                    std::span<ublk_celld const> cellds,
                                    std::span<T> cells, auto &&f,
                                    auto &&...args) {
  if (auto cellds_left = cellds_nr; cellds_left > 0 && celldn < cellds.size())
      [[likely]] {
    for (auto const *celld = &cellds[celldn];
         cellds_left > 0 && celldn < cellds.size();
         celldn = celld->ncelld, celld = &cellds[celldn], --cellds_left) {

      if (!(celld->offset + celld->data_sz <= cells.size())) [[unlikely]]
        return EINVAL;

      if (auto const r = f(cells.subspan(celld->offset, celld->data_sz),
                           std::forward<decltype(args)>(args)...))
        return r;
    }
  }
  return 0;
};
}
