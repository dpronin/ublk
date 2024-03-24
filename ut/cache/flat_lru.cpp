#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>

#include <algorithm>

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

TEST(Cache_FlatLRU, InsertAndFind) {
  constexpr auto kCacheLenMax{5uz};
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
        auto buf_dup{mm::make_unique_for_overwrite_bytes(kCacheItemSz)};
        std::copy_n(buf_pair.second.get(), kCacheItemSz, buf_dup.get());
        return {buf_pair.first, std::move(buf_dup)};
      });

  auto rd{std::random_device{}};
  std::ranges::shuffle(bufs_pairs, rd);

  for (auto &buf_pair : bufs_pairs) {
    auto const evicted_value{cache->update(std::move(buf_pair))};
    EXPECT_FALSE(evicted_value.has_value());
  }

  for (auto const &[key, buf_dup] : bufs_pairs_dup) {
    auto const buf{cache->find(key)};
    EXPECT_EQ(buf.size(), kCacheItemSz);
    EXPECT_THAT(buf, ElementsAreArray(buf_dup.get(), kCacheItemSz));
  }
}

} // namespace ublk::ut::cache
