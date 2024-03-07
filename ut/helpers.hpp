#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>

#include <algorithm>
#include <memory>
#include <random>
#include <span>

#include "read_query.hpp"
#include "write_query.hpp"

namespace ublk::ut {

inline std::unique_ptr<std::byte[]> make_unique_random_bytes(size_t sz) {
  auto storage{std::make_unique_for_overwrite<std::byte[]>(sz)};
  std::generate_n(storage.get(), sz, [rd = std::random_device{}] mutable {
    return static_cast<std::byte>(rd());
  });
  return storage;
}

inline auto make_inmem_writer(std::span<std::byte> storage) noexcept {
  return [=](std::shared_ptr<write_query> wq) {
    EXPECT_TRUE(static_cast<bool>(wq));
    EXPECT_FALSE(wq->buf().empty());
    EXPECT_LE(wq->buf().size() + wq->offset(), storage.size());
    std::ranges::copy(wq->buf(), storage.data() + wq->offset());
    return 0;
  };
};

inline auto make_inmem_reader(std::span<std::byte const> storage) noexcept {
  return [=](std::shared_ptr<read_query> rq) {
    EXPECT_TRUE(static_cast<bool>(rq));
    EXPECT_FALSE(rq->buf().empty());
    EXPECT_LE(rq->buf().size() + rq->offset(), storage.size());
    std::ranges::copy(storage.subspan(rq->offset(), rq->buf().size()),
                      rq->buf().begin());
    return 0;
  };
};

} // namespace ublk::ut
