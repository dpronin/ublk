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

#include "utils/concepts.hpp"
#include "utils/span.hpp"

namespace ublk::cache {

template <std::unsigned_integral Key, typename T> class flat_lru {
public:
  static std::unique_ptr<flat_lru<Key, T>> create(uint64_t cache_len,
                                                  uint64_t cache_item_sz) {
    auto cache{std::unique_ptr<flat_lru<Key, T>>{}};
    if (cache_len && cache_item_sz) {
      cache = std::unique_ptr<flat_lru<Key, T>>(new flat_lru<Key, T>{
          cache_len,
          cache_item_sz,
      });
    }
    return cache;
  }

private:
  using cache_item_ref_t = uint64_t;

  using key_type = Key;
  using stored_type = struct {
    mm::uptrwd<T[]> data;
    mutable cache_item_ref_t refs;
  };
  using value_type = std::pair<key_type, stored_type>;

  static inline constexpr auto key_proj = [](value_type const &value) noexcept {
    return value.first;
  };
  using keys_cmp = std::less<>;
#ifndef NDEBUG
  using keys_eq = std::equal_to<>;
#endif

  auto cache_value_view(mm::uptrwd<T[]> const &cache_value) const noexcept {
    return std::span{cache_value.get(), item_sz()};
  }

  void invalidate(value_type &value) noexcept { value.second.refs = len_max(); }

  bool is_valid(value_type const &value) const noexcept {
    return value.second.refs != len_max();
  }

  explicit flat_lru(uint64_t len_max, uint64_t item_sz)
      : cache_len_max_(len_max), cache_item_sz_(item_sz) {
    assert(this->len_max() > 0);
    assert(this->item_sz() > 0);

    cache_.reserve(this->len_max());
  }

  void touch(size_t index) const noexcept {
    assert(index < cache_.size());

    auto const to_refs{
        [](value_type const &v) -> cache_item_ref_t & { return v.second.refs; },
    };

    auto const less_than{
        [](cache_item_ref_t eo) {
          return [=](cache_item_ref_t o) { return o < eo; };
        },
    };

    auto &eo{to_refs(cache_[index])};

    /* clang-format off */
    for (auto &o :   std::views::all(cache_)
                   | std::views::transform(to_refs)
                   | std::views::filter(less_than(eo))) {
      ++o;
    }
    /* clang-format on */

    eo = 0;
  }

  std::pair<size_t, bool> lower_bound_find(Key key) const noexcept {
    auto const value_it{
        std::ranges::lower_bound(cache_, key, keys_cmp{}, key_proj),
    };
    auto const index{value_it - cache_.begin()};
    return {
        static_cast<size_t>(index),
        cache_.end() != value_it && key == key_proj(*value_it) &&
            is_valid(cache_[index]),
    };
  }

  auto range_find(std::pair<Key, Key> range) const noexcept {
    assert(!(range.first > range.second));
    auto const first{
        std::ranges::lower_bound(cache_, range.first, keys_cmp{}, key_proj),
    };
    auto const last{
        std::ranges::lower_bound(std::ranges::subrange{first, cache_.end()},
                                 range.second, keys_cmp{}, key_proj),
    };
    return std::views::iota(first - cache_.begin(), last - cache_.begin());
  }

public:
  ~flat_lru() = default;

  flat_lru(flat_lru const &) = delete;
  flat_lru &operator=(flat_lru const &) = delete;

  flat_lru(flat_lru &&) = default;
  flat_lru &operator=(flat_lru &&) = default;

  uint64_t item_sz() const noexcept { return cache_item_sz_; }
  uint64_t len_max() const noexcept { return cache_len_max_; }

  std::span<T const> find(Key key) const noexcept {
    if (auto const [index, exact_match] = lower_bound_find(key); exact_match) {
      touch(index);
      return cache_value_view(cache_[index].second.data);
    }
    return {};
  }

  std::span<T> find_mutable(Key key) noexcept {
    return const_span_cast(find(key));
  }

  std::optional<std::pair<Key, mm::uptrwd<T[]>>>
  update(std::pair<Key, mm::uptrwd<T[]>> value) noexcept {
    auto evicted_value{std::optional<std::pair<Key, mm::uptrwd<T[]>>>{}};

    bool should_evict{true};

    auto [index, exact_match]{lower_bound_find(value.first)};
    if (!exact_match) {
      if (!(index < cache_.size()) || is_valid(cache_[index])) {
        if (!(cache_.size() < len_max())) [[likely]] {
          auto value_it = cache_.begin() + index;

          size_t evict_index{0};
          if (is_valid(cache_[evict_index])) {
            for (auto i : std::views::iota(1uz, cache_.size())) {
              if (cache_[evict_index].second.refs < cache_[i].second.refs) {
                evict_index = i;
                if (!is_valid(cache_[evict_index]))
                  break;
              }
            }
          }

          if (evict_index < index) {
            value_it = std::rotate(cache_.begin() + evict_index,
                                   cache_.begin() + evict_index + 1, value_it);
          } else {
            std::rotate(value_it, cache_.begin() + evict_index,
                        cache_.begin() + evict_index + 1);
          }

          index = value_it - cache_.begin();
        } else {
          index = cache_.emplace(cache_.begin() + index, value.first,
                                 stored_type{std::move(value.second), {}}) -
                  cache_.begin();
          invalidate(cache_[index]);
          should_evict = false;
        }
      }
    }

    if (should_evict) {
      evicted_value.emplace(
          std::exchange(cache_[index].first, value.first),
          std::exchange(cache_[index].second.data, std::move(value.second)));
    }

    touch(index);

#ifndef NDEBUG
    if (!exact_match) {
      assert(std::ranges::is_sorted(cache_, keys_cmp{}, key_proj));
      assert(cache_.end() ==
             std::ranges::adjacent_find(cache_, keys_eq{}, key_proj));
    }
#endif

    return evicted_value;
  }

  bool exists(Key key) const noexcept { return lower_bound_find(key).second; }

  void invalidate(Key key) noexcept {
    if (auto const [index, exact_match]{lower_bound_find(key)}; exact_match)
      invalidate(cache_[index]);
  }

  void invalidate_range(std::pair<Key, Key> range) noexcept {
    for (auto index : range_find(range))
      invalidate(cache_[index]);
  }

private:
  uint64_t cache_len_max_;
  uint64_t cache_item_sz_;

  std::vector<value_type> cache_;
};

} // namespace ublk::cache
