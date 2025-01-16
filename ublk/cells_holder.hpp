#pragma once

#include <cstddef>

#include <span>
#include <type_traits>

#include <gsl/assert>

#include <linux/ublkdrv/celld.h>

namespace ublk {

template <bool CellsImmutable = false> class cells_holder {
public:
  auto cellds() const noexcept { return cellds_; }
  auto cells() const noexcept { return cells_; }

protected:
  cells_holder() = default;
  explicit cells_holder(
      std::span<ublkdrv_celld const> cellds,
      std::span<std::conditional_t<CellsImmutable, std::byte const, std::byte>>
          cells) noexcept
      : cellds_(cellds), cells_(cells) {
    Ensures(!cellds.empty());
    Ensures(!cells.empty());
  }
  ~cells_holder() = default;

  cells_holder(cells_holder const &) = default;
  cells_holder &operator=(cells_holder const &) = default;

  cells_holder(cells_holder &&other) = delete;
  cells_holder &operator=(cells_holder &&other) = delete;

private:
  std::span<ublkdrv_celld const> cellds_;
  std::span<std::conditional_t<CellsImmutable, std::byte const, std::byte>>
      cells_;
};

} // namespace ublk
