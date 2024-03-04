#pragma once

#include <optional>
#include <span>
#include <type_traits>

#include "utils/align.hpp"
#include "utils/concepts.hpp"

#include "cfq_base.hpp"
#include "cfq_pos.hpp"

namespace ublk {

template <cfq_suitable T, std::integral I1, std::integral I2>
class alignas(hardware_destructive_interference_size) cfq_popper
    : public cfq_base<const T, I1, const I2> {

  using base_t = cfq_base<const T, I1, const I2>;

public:
  explicit cfq_popper(pos_t<I1> p_head, pos_t<const I2> p_tail,
                      std::span<const T> items)
      : base_t(std::move(p_head), std::move(p_tail), items) {}
  ~cfq_popper() = default;

  cfq_popper(cfq_popper const &) = delete;
  cfq_popper operator=(cfq_popper const &) = delete;

  cfq_popper(cfq_popper &&) = delete;
  cfq_popper &operator=(cfq_popper &&) = delete;

  std::optional<T> pop() noexcept(std::is_nothrow_copy_constructible_v<T> &&
                                  std::is_nothrow_destructible_v<T>) {
    std::optional<T> v;

    auto ph = __atomic_load_n(this->ph_.get(), __ATOMIC_RELAXED);
    do {
      if (auto const pt = __atomic_load_n(this->pt_.get(), __ATOMIC_ACQUIRE);
          pt == ph) [[unlikely]] {

        v.reset();
        break;
      }
      v = this->items_[ph];
    } while (!__atomic_compare_exchange_n(
        this->ph_.get(), &ph, (ph + 1) % this->capacity_full(), true,
        __ATOMIC_RELEASE, __ATOMIC_ACQUIRE));

    return v;
  }
};

} // namespace ublk
