#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>

#include <algorithm>
#include <concepts>
#include <functional>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <utility>
#include <vector>

#include "mm/mem_types.hpp"

#include "utils/functional.hpp"
#include "utils/span.hpp"

namespace ublk::cache {

/* clang-format off */
template <
  typename Key,
  typename T,
  typename KeyCompare = std::ranges::less,
  typename KeyEqual = std::ranges::equal_to
>
/* clang-format on */
  requires std::strict_weak_order<KeyCompare, Key, Key> &&
           std::equivalence_relation<KeyEqual, Key, Key>
class flat_lru {
public:
  using key_type = Key;
  using data_type = mm::uptrwd<T[]>;
  using key_compare = KeyCompare;
  using key_equal = KeyEqual;

  static std::unique_ptr<flat_lru> create(uint64_t cache_len,
                                          uint64_t cache_item_sz);

private:
  using cache_item_ref_t = uint64_t;
  using stored_type = struct {
    data_type data;
    mutable cache_item_ref_t refs;
  };
  using value_type = std::pair<key_type, stored_type>;

  static inline constexpr auto cache_item_to_key_proj =
      [](value_type const &value) { return value.first; };

public:
  explicit flat_lru(uint64_t len_max, uint64_t item_sz)
      : cache_len_max_(len_max), cache_item_sz_(item_sz) {
    assert(this->len_max() > 0);
    assert(this->item_sz() > 0);

    cache_.reserve(this->len_max());
  }
  ~flat_lru() = default;

  flat_lru(flat_lru const &) = delete;
  flat_lru &operator=(flat_lru const &) = delete;

  flat_lru(flat_lru &&) = default;
  flat_lru &operator=(flat_lru &&) = default;

  uint64_t item_sz() const { return cache_item_sz_; }
  uint64_t len_max() const { return cache_len_max_; }

  std::span<T const> find(key_type const &key) const {
    if (auto const [index, exact_match] = lower_bound_find(key); exact_match) {
      touch(index);
      return data_view(cache_[index]);
    }
    return {};
  }

  std::span<T> find_mutable(key_type const &key) {
    return const_span_cast(find(key));
  }

  std::optional<std::pair<key_type, data_type>>
  update(std::pair<key_type, data_type> value);

  bool exists(key_type const &key) const {
    return lower_bound_find(key).second;
  }

  void invalidate(key_type const &key) {
    if (auto const [index, exact_match]{lower_bound_find(key)}; exact_match)
      invalidate(cache_[index]);
  }

  void invalidate_range(std::pair<key_type, key_type> const &range) {
    for (auto index : range_find(range))
      invalidate(cache_[index]);
  }

private:
  auto data_view(value_type const &value) const {
    return std::span{value.second.data.get(), item_sz()};
  }

  void invalidate(value_type &value) {
    value.second.refs = len_max();
    value.second.data.reset();
  }

  bool is_valid(value_type const &value) const {
    return value.second.refs != len_max();
  }

  void touch(size_t index) const;

  std::pair<size_t, bool> lower_bound_find(key_type const &key) const;

  auto range_find(std::pair<key_type, key_type> const &range) const;

  size_t evict_index_find() const;

  uint64_t cache_len_max_;
  uint64_t cache_item_sz_;

  std::vector<value_type> cache_;
};

template <typename Key, typename T, typename KeyCompare, typename KeyEqual>
  requires std::strict_weak_order<KeyCompare, Key, Key> &&
           std::equivalence_relation<KeyEqual, Key, Key>
auto flat_lru<Key, T, KeyCompare, KeyEqual>::range_find(
    std::pair<key_type, key_type> const &range) const {
  assert(range.first < range.second);
  auto const first{
      std::ranges::lower_bound(cache_, range.first, key_compare{},
                               cache_item_to_key_proj),
  };
  auto const last{
      std::ranges::lower_bound(std::ranges::subrange{first, cache_.end()},
                               range.second, key_compare{},
                               cache_item_to_key_proj),
  };
  return std::views::iota(first - cache_.begin(), last - cache_.begin());
}

template <typename Key, typename T, typename KeyCompare, typename KeyEqual>
  requires std::strict_weak_order<KeyCompare, Key, Key> &&
           std::equivalence_relation<KeyEqual, Key, Key>
size_t flat_lru<Key, T, KeyCompare, KeyEqual>::evict_index_find() const {
  auto evict_index{0uz};
  if (is_valid(cache_[evict_index])) {
    for (auto index : std::views::iota(1uz, cache_.size())) {
      if (cache_[evict_index].second.refs < cache_[index].second.refs) {
        evict_index = index;
        if (!is_valid(cache_[evict_index]))
          break;
      }
    }
  }
  return evict_index;
}

template <typename Key, typename T, typename KeyCompare, typename KeyEqual>
  requires std::strict_weak_order<KeyCompare, Key, Key> &&
           std::equivalence_relation<KeyEqual, Key, Key>
std::pair<size_t, bool>
flat_lru<Key, T, KeyCompare, KeyEqual>::lower_bound_find(
    key_type const &key) const {
  auto const value_it{
      std::ranges::lower_bound(cache_, key, key_compare{},
                               cache_item_to_key_proj),
  };
  auto const index{value_it - cache_.begin()};
  return {
      static_cast<size_t>(index),
      cache_.end() != value_it &&
          key_equal{}(key, cache_item_to_key_proj(*value_it)) &&
          is_valid(cache_[index]),
  };
}

template <typename Key, typename T, typename KeyCompare, typename KeyEqual>
  requires std::strict_weak_order<KeyCompare, Key, Key> &&
               std::equivalence_relation<KeyEqual, Key, Key>
auto flat_lru<Key, T, KeyCompare, KeyEqual>::create(
    uint64_t cache_len, uint64_t cache_item_sz) -> std::unique_ptr<flat_lru> {
  auto cache{std::unique_ptr<flat_lru>{}};
  if (cache_len && cache_item_sz) {
    cache = std::unique_ptr<flat_lru>{
        new flat_lru{
            cache_len,
            cache_item_sz,
        },
    };
  }
  return cache;
}

template <typename Key, typename T, typename KeyCompare, typename KeyEqual>
  requires std::strict_weak_order<KeyCompare, Key, Key> &&
           std::equivalence_relation<KeyEqual, Key, Key>
void flat_lru<Key, T, KeyCompare, KeyEqual>::touch(size_t index) const {
  assert(index < cache_.size());

  auto const to_refs{
      [](value_type const &v) -> cache_item_ref_t & { return v.second.refs; },
  };

  auto &eo{to_refs(cache_[index])};

  /* clang-format off */
  for (auto &o :   std::views::all(cache_)
                 | std::views::transform(to_refs)
                 | std::views::filter(make_less_than(eo))) {
    ++o;
  }
  /* clang-format on */

  eo = 0;
}

template <typename Key, typename T, typename KeyCompare, typename KeyEqual>
  requires std::strict_weak_order<KeyCompare, Key, Key> &&
               std::equivalence_relation<KeyEqual, Key, Key>
auto flat_lru<Key, T, KeyCompare, KeyEqual>::update(
    std::pair<key_type, data_type> value)
    -> std::optional<std::pair<key_type, data_type>> {
  auto evicted_value{std::optional<std::pair<key_type, data_type>>{}};

  bool should_evict{true};

  auto [index, exact_match]{lower_bound_find(value.first)};
  if (!exact_match) {
    if (!(index < cache_.size()) || is_valid(cache_[index])) {
      if (!(cache_.size() < len_max())) [[likely]] {
        auto value_it = cache_.begin() + index;

        if (auto const evict_index{evict_index_find()}; evict_index < index) {
          value_it = std::rotate(cache_.begin() + evict_index,
                                 cache_.begin() + evict_index + 1, value_it);
        } else {
          std::rotate(value_it, cache_.begin() + evict_index,
                      cache_.begin() + evict_index + 1);
        }

        index = value_it - cache_.begin();
      } else {
        cache_.emplace(cache_.begin() + index,
                       std::move_if_noexcept(value.first), stored_type{});
        invalidate(cache_[index]);
        cache_[index].second.data = std::move(value.second);
        should_evict = false;
      }
    }
  }

  if (should_evict) {
    evicted_value.emplace(
        std::exchange(cache_[index].first, std::move_if_noexcept(value.first)),
        std::exchange(cache_[index].second.data, std::move(value.second)));
  }

  touch(index);

#ifndef NDEBUG
  if (!exact_match) {
    assert(
        std::ranges::is_sorted(cache_, key_compare{}, cache_item_to_key_proj));
    assert(cache_.end() == std::ranges::adjacent_find(cache_, key_equal{},
                                                      cache_item_to_key_proj));
  }
#endif

  return evicted_value;
}

} // namespace ublk::cache
