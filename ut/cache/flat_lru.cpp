#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>

#include <algorithm>
#include <limits>
#include <random>
#include <ranges>
#include <vector>

#include "mm/mem.hpp"

#include "cache/flat_lru.hpp"

using namespace testing;

namespace ublk::ut::cache {

TEST(Cache_FlatLRU, CreateValid) {
  constexpr auto kCacheLenMax{1uz};
  constexpr auto kCacheItemSz{1uz};

  auto cache_not_null{
      ublk::cache::flat_lru<uint64_t, std::byte>::create(kCacheLenMax,
                                                         kCacheItemSz),
  };
  ASSERT_TRUE(cache_not_null);

  EXPECT_EQ(cache_not_null->len_max(), kCacheLenMax);
  EXPECT_EQ(cache_not_null->item_sz(), kCacheItemSz);
}

TEST(Cache_FlatLRU, CreateInvalid) {
  constexpr auto kCacheLenMax{1uz};
  constexpr auto kCacheItemSz{1uz};

  auto cache_nulls{
      std::array{
          ublk::cache::flat_lru<uint64_t, std::byte>::create(0uz, kCacheItemSz),
          ublk::cache::flat_lru<uint64_t, std::byte>::create(kCacheLenMax, 0uz),
      },
  };

  for (auto const &cache_null : cache_nulls)
    EXPECT_FALSE(cache_null);
}

TEST(Cache_FlatLRU, InsertAndFindNonExistent) {
  constexpr auto kCacheLenMax{32uz};
  constexpr auto kCacheItemSz{1u};

  auto cache{
      ublk::cache::flat_lru<uint64_t, std::byte>::create(kCacheLenMax,
                                                         kCacheItemSz),
  };
  ASSERT_TRUE(cache);

  for (auto key : std::views::iota(0uz, kCacheLenMax)) {
    auto const evicted_value{cache->update({
        key,
        mm::make_unique_for_overwrite_bytes(kCacheItemSz),
    })};
  }

  auto rd{std::random_device{}};
  auto unexistent_keys_random_gen{
      std::uniform_int_distribution<uint64_t>{
          kCacheLenMax,
          std::numeric_limits<uint64_t>::max(),
      },
  };

  for (auto i [[maybe_unused]] : std::views::iota(0, 1024)) {
    auto const unexistent_key{unexistent_keys_random_gen(rd)};
    EXPECT_FALSE(cache->exists(unexistent_key));
    auto const buf{cache->find(unexistent_key)};
    EXPECT_TRUE(buf.empty());
  }
}

TEST(Cache_FlatLRU, InsertAndFind) {
  constexpr auto kCacheLenMax{32uz};
  constexpr auto kCacheItemSz{16uz};

  auto cache{
      ublk::cache::flat_lru<uint64_t, std::byte>::create(kCacheLenMax,
                                                         kCacheItemSz),
  };
  ASSERT_TRUE(cache);

  auto bufs_pairs{
      std::vector<std::pair<uint64_t, std::unique_ptr<std::byte[]>>>{
          cache->len_max(),
      },
  };
  std::ranges::generate(
      bufs_pairs, [i = 0uz] mutable -> decltype(bufs_pairs)::value_type {
        return {i++, mm::make_unique_randomized_bytes(kCacheItemSz)};
      });

  decltype(bufs_pairs) bufs_pairs_dup{cache->len_max()};
  std::ranges::transform(
      bufs_pairs, bufs_pairs_dup.begin(),
      [](auto const &buf_pair) -> decltype(bufs_pairs)::value_type {
        return {
            buf_pair.first,
            mm::duplicate_unique(buf_pair.second, kCacheItemSz),
        };
      });

  auto rd{std::random_device{}};
  std::ranges::shuffle(bufs_pairs, rd);

  for (auto &buf_pair : bufs_pairs) {
    auto const evicted_value{cache->update(std::move(buf_pair))};
    EXPECT_FALSE(evicted_value.has_value());
  }

  for (auto const &[key, buf_dup] : bufs_pairs_dup) {
    auto const buf{cache->find(key)};
    EXPECT_THAT(buf, ElementsAreArray(buf_dup.get(), kCacheItemSz));
  }
}

TEST(Cache_FlatLRU, FindMutableAndCheckMutation) {
  constexpr auto kCacheLenMax{32uz};
  constexpr auto kCacheItemSz{16uz};

  auto cache{
      ublk::cache::flat_lru<uint64_t, std::byte>::create(kCacheLenMax,
                                                         kCacheItemSz),
  };
  ASSERT_TRUE(cache);

  auto bufs_pairs{
      std::vector<std::pair<uint64_t, std::unique_ptr<std::byte[]>>>{
          cache->len_max(),
      },
  };
  std::ranges::generate(
      bufs_pairs, [i = 0uz] mutable -> decltype(bufs_pairs)::value_type {
        return {i++, mm::make_unique_for_overwrite_bytes(kCacheItemSz)};
      });

  decltype(bufs_pairs) bufs_pairs_dup{cache->len_max()};
  std::ranges::transform(
      bufs_pairs, bufs_pairs_dup.begin(),
      [](auto const &buf_pair) -> decltype(bufs_pairs)::value_type {
        return {buf_pair.first, std::unique_ptr<std::byte[]>{}};
      });

  auto rd{std::random_device{}};
  std::ranges::shuffle(bufs_pairs, rd);

  for (auto &buf_pair : bufs_pairs) {
    auto const evicted_value{cache->update(std::move(buf_pair))};
    EXPECT_FALSE(evicted_value.has_value());
  }

  for (auto &[key, buf_dup] : bufs_pairs_dup) {
    auto const buf{cache->find_mutable(key)};
    ASSERT_FALSE(buf.empty());
    ASSERT_EQ(buf.size(), kCacheItemSz);
    std::copy_n(mm::make_unique_randomized_bytes(kCacheItemSz).get(),
                kCacheItemSz, buf.begin());
    buf_dup = mm::make_unique_for_overwrite_bytes(kCacheItemSz);
    std::ranges::copy(buf, buf_dup.get());
  }

  for (auto const &[key, buf_dup] : bufs_pairs_dup) {
    auto const buf{cache->find(key)};
    EXPECT_THAT(buf, ElementsAreArray(buf_dup.get(), kCacheItemSz));
  }
}

TEST(Cache_FlatLRU, Invalidate) {
  constexpr auto kCacheLenMax{32uz};
  constexpr auto kCacheItemSz{1uz};

  auto cache{
      ublk::cache::flat_lru<uint64_t, std::byte>::create(kCacheLenMax,
                                                         kCacheItemSz),
  };
  ASSERT_TRUE(cache);

  auto bufs_pairs{
      std::vector<std::pair<uint64_t, std::unique_ptr<std::byte[]>>>{
          cache->len_max(),
      },
  };
  std::ranges::generate(
      bufs_pairs, [i = 0uz] mutable -> decltype(bufs_pairs)::value_type {
        return {i++, mm::make_unique_for_overwrite_bytes(kCacheItemSz)};
      });

  auto rd{std::random_device{}};
  std::ranges::shuffle(bufs_pairs, rd);

  for (auto &[key, buf] : bufs_pairs)
    cache->update({key, std::move(buf)});

  for (auto keys{std::vector<uint64_t>{}}; !bufs_pairs.empty();) {
    for (auto key : bufs_pairs | std::views::keys)
      EXPECT_TRUE(cache->exists(key));

    auto const key{bufs_pairs.back().first};
    bufs_pairs.pop_back();

    cache->invalidate(key);

    keys.push_back(key);
    for (auto key_prev : keys | std::views::reverse)
      EXPECT_FALSE(cache->exists(key_prev));
  }
}

TEST(Cache_FlatLRU, InvalidateRange) {
  constexpr auto kCacheLenMax{32uz};
  constexpr auto kCacheItemSz{1uz};

  auto cache{
      ublk::cache::flat_lru<uint64_t, std::byte>::create(kCacheLenMax,
                                                         kCacheItemSz),
  };
  ASSERT_TRUE(cache);

  auto bufs_pairs{
      std::vector<std::pair<uint64_t, std::unique_ptr<std::byte[]>>>{
          cache->len_max(),
      },
  };
  std::ranges::generate(
      bufs_pairs, [i = 0uz] mutable -> decltype(bufs_pairs)::value_type {
        return {i++, mm::make_unique_for_overwrite_bytes(kCacheItemSz)};
      });

  auto rd{std::random_device{}};
  std::ranges::shuffle(bufs_pairs, rd);

  for (auto &[key, buf] : bufs_pairs)
    cache->update({key, std::move(buf)});

  for (auto key : bufs_pairs | std::views::keys)
    EXPECT_TRUE(cache->exists(key));

  cache->invalidate_range({0uz, bufs_pairs.size()});

  for (auto key : bufs_pairs | std::views::keys)
    EXPECT_FALSE(cache->exists(key));
}

} // namespace ublk::ut::cache
