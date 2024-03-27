#pragma once

#include <cassert>
#include <cstddef>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <span>
#include <vector>

#include "mm/mem.hpp"

#include "read_query.hpp"
#include "rw_handler_interface.hpp"
#include "write_query.hpp"

namespace ublk::ut {

class MockRWHandler : public IRWHandler {
public:
  MOCK_METHOD(int, submit, (std::shared_ptr<read_query>), (noexcept));
  MOCK_METHOD(int, submit, (std::shared_ptr<write_query>), (noexcept));
};

inline auto make_inmem_writer(std::span<std::byte> storage) noexcept {
  return [=](std::shared_ptr<write_query> wq) {
    assert(wq);
    EXPECT_FALSE(wq->buf().empty());
    EXPECT_LE(wq->buf().size() + wq->offset(), storage.size());
    std::ranges::copy(wq->buf(), storage.data() + wq->offset());
    return 0;
  };
};

inline auto make_inmem_reader(std::span<std::byte const> storage) noexcept {
  return [=](std::shared_ptr<read_query> rq) {
    assert(rq);
    EXPECT_FALSE(rq->buf().empty());
    EXPECT_LE(rq->buf().size() + rq->offset(), storage.size());
    std::ranges::copy(storage.subspan(rq->offset(), rq->buf().size()),
                      rq->buf().begin());
    return 0;
  };
};

template <is_byte T>
inline auto storages_to_spans(std::vector<std::unique_ptr<T[]>> const &storages,
                              size_t storage_sz) {
  std::vector<std::span<T>> storage_spans{storages.size()};
  std::ranges::transform(storages, storage_spans.begin(),
                         [storage_sz](auto const &storage) -> std::span<T> {
                           return {storage.get(), storage_sz};
                         });
  return storage_spans;
}

template <is_byte T>
inline auto
storages_to_const_spans(std::vector<std::unique_ptr<T[]>> const &storages,
                        size_t storage_sz) {
  std::vector<std::span<T const>> storage_spans{storages.size()};
  std::ranges::transform(
      storages, storage_spans.begin(),
      [storage_sz](auto const &storage) -> std::span<T const> {
        return {storage.get(), storage_sz};
      });
  return storage_spans;
}

inline auto make_unique_for_overwrite_storage(size_t storage_sz) {
  return mm::make_unique_for_overwrite_bytes(storage_sz);
}

inline auto make_unique_for_overwrite_storages(size_t storage_sz, size_t nr) {
  std::vector<std::unique_ptr<std::byte[]>> storages{nr};
  std::ranges::generate(storages, [storage_sz] {
    return make_unique_for_overwrite_storage(storage_sz);
  });
  return storages;
}

inline auto make_unique_randomized_storage(size_t storage_sz) {
  return mm::make_unique_randomized_bytes(storage_sz);
}

inline auto make_unique_randomized_storages(size_t storage_sz, size_t nr) {
  std::vector<std::unique_ptr<std::byte[]>> storages{nr};
  std::ranges::generate(storages, [storage_sz] {
    return make_unique_randomized_storage(storage_sz);
  });
  return storages;
}

inline auto make_unique_zeroed_storage(size_t storage_sz) {
  return mm::make_unique_zeroed_bytes(storage_sz);
}

inline auto make_unique_zeroed_storages(size_t storage_sz, size_t nr) {
  std::vector<std::unique_ptr<std::byte[]>> storages{nr};
  std::ranges::generate(storages, [storage_sz] {
    return make_unique_zeroed_storage(storage_sz);
  });
  return storages;
}

} // namespace ublk::ut
