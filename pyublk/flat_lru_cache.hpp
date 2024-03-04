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

#include "mm/mem.hpp"
#include "mm/mem_types.hpp"

#include "concepts.hpp"
#include "sector.hpp"
#include "span.hpp"

namespace ublk {

template <std::unsigned_integral Key, typename T> class flat_lru_cache {
public:
  static std::unique_ptr<flat_lru_cache<Key, T>>
  create(uint64_t cache_len, uint64_t cache_item_sz) {
    auto cache = std::unique_ptr<flat_lru_cache<Key, T>>{};
    if (cache_len && cache_item_sz) {
      auto cache_storage = std::make_unique<mm::uptrwd<T[]>[]>(cache_len);
      std::ranges::generate(
          std::span{cache_storage.get(), cache_len},
          mm::get_unique_bytes_generator(kSectorSz, cache_item_sz));
      cache =
          std::unique_ptr<flat_lru_cache<Key, T>>(new flat_lru_cache<Key, T>{
              std::move(cache_storage),
              cache_len,
              cache_item_sz,
          });
    }
    return cache;
  }

private:
  using cache_item_t = std::pair<Key, mm::uptrwd<T[]>>;
  using cache_item_ref_t = uint64_t;

  static inline constexpr auto key_proj =
      [](cache_item_t const &value) noexcept { return value.first; };
  using keys_cmp = std::less<>;
#ifndef NDEBUG
  using keys_eq = std::equal_to<>;
#endif

  auto cache_view() const noexcept { return std::span{cache_.get(), len()}; }

  auto cache_value_view(mm::uptrwd<T[]> const &cache_value) const noexcept {
    return std::span{
        cache_value.get(),
        item_sz(),
    };
  }

  bool is_valid_ref(cache_item_ref_t refs) const noexcept {
    return refs != len();
  }

  bool is_valid(size_t index) const noexcept {
    assert(index < len());
    return is_valid_ref(cache_refs_[index]);
  }

  explicit flat_lru_cache(std::unique_ptr<mm::uptrwd<T[]>[]> storage,
                          uint64_t storage_len, uint64_t storage_item_sz)
      : cache_len_(storage_len), cache_item_sz_(storage_item_sz) {
    assert(storage);
    assert(len() > 0);
    assert(item_sz() > 0);

    cache_ = std::make_unique<cache_item_t[]>(len());

    std::transform(
        storage.get(), storage.get() + storage_len, cache_.get(),
        [key_init = Key{}](auto &storage_item) mutable -> cache_item_t {
          return {
              Key{key_init++},
              std::move(storage_item),
          };
        });

    assert(std::ranges::all_of(cache_view(), [](auto const &cache_item) {
      return static_cast<bool>(cache_item.second);
    }));

    cache_refs_ = std::make_unique<cache_item_ref_t[]>(len());
    std::fill_n(cache_refs_.get(), len(), len());
  }

  void touch(size_t index) const noexcept {
    assert(index != len());

    auto const eo = cache_refs_[index];
    for (size_t i = 0; i < len(); ++i) {
      if (auto &o = cache_refs_[i]; o < eo)
        ++o;
    }
    cache_refs_[index] = 0;
  }

  std::pair<size_t, bool> lower_bound_find(Key key) const noexcept {
    auto const cache = cache_view();
    auto const value_it =
        std::ranges::lower_bound(cache, key, keys_cmp{}, key_proj);
    size_t const index = value_it - cache.begin();
    return {
        index,
        cache.end() != value_it && key == key_proj(*value_it) &&
            is_valid(index),
    };
  }

  std::pair<size_t, size_t>
  range_find(std::pair<Key, Key> range) const noexcept {
    assert(!(range.first > range.second));
    auto const cache = cache_view();
    auto const first =
        std::ranges::lower_bound(cache, range.first, keys_cmp{}, key_proj);
    auto const last =
        std::ranges::upper_bound(std::ranges::subrange{first, cache.end()},
                                 range.second, keys_cmp{}, key_proj);
    return {first - cache.begin(), last - cache.begin()};
  }

public:
  ~flat_lru_cache() = default;

  flat_lru_cache(flat_lru_cache const &) = delete;
  flat_lru_cache &operator=(flat_lru_cache const &) = delete;

  flat_lru_cache(flat_lru_cache &&) = default;
  flat_lru_cache &operator=(flat_lru_cache &&) = default;

  uint64_t item_sz() const noexcept { return cache_item_sz_; }
  uint64_t len() const noexcept { return cache_len_; }

  std::span<T const> find(Key key) const noexcept {
    if (auto const [index, exact_match] = lower_bound_find(key); exact_match) {
      touch(index);
      return cache_value_view(cache_[index].second);
    }
    return {};
  }

  std::span<T> find_mutable(Key key) noexcept {
    return const_span_cast(find(key));
  }

  std::optional<std::pair<Key, mm::uptrwd<T[]>>>
  update(std::pair<Key, mm::uptrwd<T[]>> value) noexcept {
    auto evicted_value = std::optional<std::pair<Key, mm::uptrwd<T[]>>>{};

    auto const cache = cache_view();

    auto [index, exact_match] = lower_bound_find(value.first);
    if (!exact_match) {
      if (!(index < cache.size()) || is_valid(index)) {
        auto value_it = cache.begin() + index;

        size_t evict_index{0};
        if (is_valid_ref(cache_refs_[evict_index])) {
          for (size_t i = 1; i < len(); ++i) {
            if (cache_refs_[evict_index] < cache_refs_[i]) {
              evict_index = i;
              if (!is_valid_ref(cache_refs_[evict_index]))
                break;
            }
          }
        }

        if (evict_index < index) {
          value_it = std::rotate(cache.begin() + evict_index,
                                 cache.begin() + evict_index + 1, value_it);
        } else {
          std::rotate(value_it, cache.begin() + evict_index,
                      cache.begin() + evict_index + 1);
        }

        index = value_it - cache.begin();
      }
    }

    evicted_value.emplace(
        std::exchange(cache[index].first, value.first),
        std::exchange(cache[index].second, std::move(value.second)));
    touch(index);

#ifndef NDEBUG
    if (!exact_match) {
      assert(std::ranges::is_sorted(cache, keys_cmp{}, key_proj));
      assert(cache.end() ==
             std::ranges::adjacent_find(cache, keys_eq{}, key_proj));
    }
#endif

    return evicted_value;
  }

  bool exists(Key key) const noexcept { return lower_bound_find(key).second; }

  void invalidate(Key key) noexcept {
    if (auto const [index, exact_match] = lower_bound_find(key); exact_match)
      cache_refs_[index] = len();
  }

  void invalidate_range(std::pair<Key, Key> range) noexcept {
    for (auto [fi, li] = range_find(range); fi < li; ++fi)
      cache_refs_[fi] = len();
  }

private:
  uint64_t cache_len_;
  uint64_t cache_item_sz_;

  std::unique_ptr<cache_item_t[]> cache_;
  std::unique_ptr<cache_item_ref_t[]> cache_refs_;
};

} // namespace ublk
