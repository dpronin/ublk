#pragma once

#include <concepts>
#include <span>
#include <stdexcept>
#include <utility>

#include "utils/concepts.hpp"

#include "pos.hpp"

namespace ublk::cfq {

template <cfq_suitable T, std::integral I1, std::integral I2> class base {
public:
  [[nodiscard]] auto capacity_full() const noexcept { return items_.size(); }
  [[nodiscard]] auto capacity() const noexcept { return capacity_full() - 1; }

protected:
  explicit base(pos_t<I1> p_head, pos_t<I2> p_tail, std::span<T> items)
      : ph_(std::move(p_head)), pt_(std::move(p_tail)), items_(items) {
    if (!ph_)
      throw std::invalid_argument("head given cannot be empty");
    if (!pt_)
      throw std::invalid_argument("tail given cannot be empty");
    if (items_.size() < 2)
      throw std::invalid_argument(
          "items given must contain at least 2 elements");
    if (*ph_ > capacity_full())
      throw std::invalid_argument("head cannot index an item out of range");
    if (*pt_ > capacity_full())
      throw std::invalid_argument("tail cannot index an item out of range");
  }
  ~base() = default;

  base(base const &) = delete;
  base operator=(base const &) = delete;

  base(base &&) = delete;
  base &operator=(base &&) = delete;

protected:
  pos_t<I1> ph_;
  pos_t<I2> pt_;
  std::span<T> items_;
};

} // namespace ublk::cfq
