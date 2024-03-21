#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>

#include <algorithm>
#include <functional>
#include <memory>
#include <span>
#include <vector>

#include "utils/random.hpp"

#include "read_query.hpp"
#include "rw_handler_interface.hpp"
#include "write_query.hpp"

namespace ublk::ut {

class MockRWHandler : public IRWHandler {
public:
  MOCK_METHOD(int, submit, (std::shared_ptr<read_query>), (noexcept));
  MOCK_METHOD(int, submit, (std::shared_ptr<write_query>), (noexcept));
};

inline std::unique_ptr<std::byte[]> make_unique_random_bytes(size_t sz) {
  auto storage{std::make_unique_for_overwrite<std::byte[]>(sz)};
  auto gen = make_random_bytes_generator();
  std::generate_n(storage.get(), sz, std::ref(gen));
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

template <is_byte T>
inline auto
make_storage_spans(std::vector<std::unique_ptr<T[]>> const &storages,
                   size_t storage_sz) {
  std::vector<std::span<T>> storage_spans{storages.size()};
  std::ranges::transform(storages, storage_spans.begin(),
                         [storage_sz](auto const &storage) -> std::span<T> {
                           return {storage.get(), storage_sz};
                         });
  return storage_spans;
}

template <is_byte T>
inline auto make_randomized_storages(size_t storage_sz, size_t nr) {
  std::vector<std::unique_ptr<T[]>> storages{nr};
  std::ranges::generate(storages, [storage_sz] {
    return ut::make_unique_random_bytes(storage_sz);
  });
  return storages;
}

template <is_byte T> inline auto make_zeroed_storage(size_t storage_sz) {
  return std::make_unique<T[]>(storage_sz);
}

template <is_byte T>
inline auto make_zeroed_storages(size_t storage_sz, size_t nr) {
  std::vector<std::unique_ptr<T[]>> storages{nr};
  std::ranges::generate(
      storages, [storage_sz] { return make_zeroed_storage<T>(storage_sz); });
  return storages;
}

} // namespace ublk::ut
