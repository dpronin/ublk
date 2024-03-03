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
  using cache_item_t = std::tuple<Key, uint64_t, mm::uptrwd<T[]>>;

  static inline constexpr auto key_proj = [](auto &&value) noexcept {
    return std::get<0>(std::forward<decltype(value)>(value));
  };
  using keys_cmp = std::less<>;
  using keys_eq = std::equal_to<>;

  auto cache_view() const noexcept {
    return std::span{cache_.get(), cache_len_};
  }

  bool is_valid(cache_item_t const &cache_item) const noexcept {
    return std::get<1>(cache_item) != cache_view().size();
  }

  auto cache_value_view(mm::uptrwd<T[]> const &cache_value) const noexcept {
    return std::span{
        cache_value.get(),
        cache_item_sz_,
    };
  }

  explicit flat_lru_cache(std::unique_ptr<mm::uptrwd<T[]>[]> storage,
                          uint64_t storage_len, uint64_t storage_item_sz)
      : cache_len_(storage_len), cache_item_sz_(storage_item_sz) {
    assert(storage);
    assert(cache_len_ > 0);
    assert(cache_item_sz_ > 0);

    cache_ = std::make_unique<cache_item_t[]>(cache_len_);

    std::ranges::transform(
        std::span{storage.get(), storage_len}, cache_.get(),
        [this, key_init = Key{}](auto &storage_item) mutable -> cache_item_t {
          return {
              Key{key_init++},
              cache_len_,
              std::move(storage_item),
          };
        });

    assert(std::ranges::all_of(cache_view(), [](auto const &cache_item) {
      return static_cast<bool>(std::get<2>(cache_item));
    }));
  }

  void touch(size_t cache_item_id) const noexcept {
    auto const cache = cache_view();

    assert(cache_item_id != cache.size());

    auto const eo = std::get<1>(cache[cache_item_id]);
    for (auto &cache_item : cache_view()) {
      if (auto &o = std::get<1>(cache_item); o < eo)
        ++o;
    }
    std::get<1>(cache[cache_item_id]) = 0;
  }

  std::pair<size_t, bool> lower_bound_find(Key key) const noexcept {
    auto const cache = cache_view();
    auto const value_it =
        std::ranges::lower_bound(cache, key, keys_cmp{}, key_proj);
    return {
        value_it - cache.begin(),
        cache.end() != value_it && key == key_proj(*value_it) &&
            is_valid(*value_it),
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
  uint64_t len() const noexcept { return cache_view().size(); }

  std::span<T const> find(Key key) const noexcept {
    if (auto const [index, exact_match] = lower_bound_find(key); exact_match) {
      touch(index);
      return cache_value_view(std::get<2>(cache_view()[index]));
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
      if (!(index < cache.size()) || is_valid(cache[index])) {
        auto value_it = cache.begin() + index;
        if (auto evict_value_it = std::ranges::max_element(
                cache, keys_cmp{},
                [](auto const &cache_item) { return std::get<1>(cache_item); });
            evict_value_it < value_it) {
          value_it = std::rotate(evict_value_it, evict_value_it + 1, value_it);
        } else {
          std::rotate(value_it, evict_value_it, evict_value_it + 1);
        }
        index = value_it - cache.begin();
      }
    }

    evicted_value.emplace(std::get<0>(cache[index]),
                          std::move(std::get<2>(cache[index])));
    std::get<0>(cache[index]) = value.first;
    std::get<2>(cache[index]) = std::move(value.second);
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
    if (auto const [index, exact_match] = lower_bound_find(key); exact_match) {
      auto const cache = cache_view();
      std::get<1>(cache[index]) = cache.size();
    }
  }

  void invalidate_range(std::pair<Key, Key> range) noexcept {
    auto const cache = cache_view();
    for (auto [fi, li] = range_find(range); fi < li; ++fi)
      std::get<1>(cache[fi]) = cache.size();
  }

private:
  uint64_t cache_len_;
  uint64_t cache_item_sz_;

  std::unique_ptr<cache_item_t[]> cache_;
};

} // namespace ublk
