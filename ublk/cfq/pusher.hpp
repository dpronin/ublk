#pragma once

#include <span>
#include <type_traits>
#include <utility>

#include "utils/align.hpp"
#include "utils/concepts.hpp"

#include "base.hpp"
#include "pos.hpp"

namespace ublk::cfq {

template <cfq_suitable T, std::integral I1, std::integral I2>
class alignas(hardware_destructive_interference_size) pusher
    : public base<T, const I1, I2> {

  using base_t = base<T, const I1, I2>;

public:
  explicit pusher(pos_t<const I1> p_head, pos_t<I2> p_tail, std::span<T> items)
      : base_t(std::move(p_head), std::move(p_tail), items) {}
  ~pusher() = default;

  pusher(pusher const &) = delete;
  pusher operator=(pusher const &) = delete;

  pusher(pusher &&) = delete;
  pusher &operator=(pusher &&) = delete;

  bool push(T const &v) noexcept(std::is_nothrow_copy_constructible_v<T>) {
    auto const pt{*this->pt_};
    auto const npt{(pt + 1) % this->capacity_full()};
    if (npt == __atomic_load_n(this->ph_.get(), __ATOMIC_RELAXED)) [[unlikely]]
      return false;

    this->items_[pt] = v;
    __atomic_store_n(this->pt_.get(), npt, __ATOMIC_RELEASE);

    return true;
  }
};

} // namespace ublk::cfq
