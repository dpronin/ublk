#pragma once

#include <cassert>

#include <span>
#include <type_traits>
#include <utility>

#include "utils/align.hpp"
#include "utils/concepts.hpp"

#include "base.hpp"
#include "pos.hpp"

namespace ublk::cfq {

template <cfq_suitable T, std::unsigned_integral I1, std::unsigned_integral I2>
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
    auto const ct{*this->pt_};
    auto const nct{(ct + 1) % this->capacity_full()};
    if (nct != __atomic_load_n(this->ph_.get(), __ATOMIC_RELAXED)) [[likely]] {
      assert(ct < this->capacity_full());
      this->items_[ct] = v;
      __atomic_store_n(this->pt_.get(), nct, __ATOMIC_RELEASE);
      return true;
    }

    return false;
  }
};

} // namespace ublk::cfq
