#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>

#include "cache/flat_lru.hpp"

using namespace testing;

constexpr auto kCacheLenMax{5uz};
constexpr auto kCacheItemSz{16uz};

namespace ublk::ut::cache {

TEST(Cache_FlatLRU, CreateAndCheck) {
  auto cache_not_null{
      ublk::cache::flat_lru<uint64_t, std::byte>::create(kCacheLenMax,
                                                         kCacheItemSz),
  };
  EXPECT_TRUE(cache_not_null);

  auto cache_nulls{
      std::array{
          ublk::cache::flat_lru<uint64_t, std::byte>::create(0uz, kCacheItemSz),
          ublk::cache::flat_lru<uint64_t, std::byte>::create(kCacheLenMax, 0uz),
      },
  };

  for (auto const &cache_null : cache_nulls)
    EXPECT_FALSE(cache_null);
}

} // namespace ublk::ut::cache
