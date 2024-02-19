#pragma once

#include <span>
#include <type_traits>
#include <utility>

#include "align.hpp"
#include "cfq_base.hpp"
#include "cfq_pos.hpp"
#include "concepts.hpp"

namespace ublk {

template <cfq_suitable T, std::integral I1, std::integral I2>
class alignas(hardware_destructive_interference_size) cfq_pusher
    : public cfq_base<T, const I1, I2> {

  using base_t = cfq_base<T, const I1, I2>;

public:
  explicit cfq_pusher(pos_t<const I1> p_head, pos_t<I2> p_tail,
                      std::span<T> items)
      : base_t(std::move(p_head), std::move(p_tail), items) {}
  ~cfq_pusher() = default;

  cfq_pusher(cfq_pusher const &) = delete;
  cfq_pusher operator=(cfq_pusher const &) = delete;

  cfq_pusher(cfq_pusher &&) = delete;
  cfq_pusher &operator=(cfq_pusher &&) = delete;

  bool push(T const &v) noexcept(std::is_nothrow_copy_constructible_v<T>) {
    auto const pt = *this->pt_;
    auto const npt = (pt + 1) % this->capacity_full();
    if (npt == __atomic_load_n(this->ph_.get(), __ATOMIC_RELAXED)) [[unlikely]]
      return false;
    this->items_[pt] = v;
    __atomic_store_n(this->pt_.get(), npt, __ATOMIC_RELEASE);
    return true;
  }
};

} // namespace ublk
