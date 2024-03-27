#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>

#include "mm/mem.hpp"

#include "utils/size_units.hpp"

#include "helpers.hpp"

#include "inmem/target.hpp"

#include "read_query.hpp"
#include "write_query.hpp"

using namespace testing;

namespace ublk::ut::inmem {

TEST(INMEM, SuccessfulReadingTheWholeStorage) {
  constexpr auto kStorageSz{4_KiB};

  auto storage{ut::make_unique_randomized_storage(kStorageSz)};
  auto *p_storage{storage.get()};

  auto tgt{ublk::inmem::Target{std::move(storage), kStorageSz}};

  auto const buf{mm::make_unique_zeroed_bytes(kStorageSz)};
  auto const buf_span{std::span{buf.get(), kStorageSz}};

  auto const res{
      tgt.process(read_query::create(
          buf_span, 0, [](read_query const &rq) { EXPECT_EQ(rq.err(), 0); })),
  };
  EXPECT_EQ(res, 0);

  EXPECT_THAT(buf_span, ElementsAreArray(p_storage, kStorageSz));
}

TEST(INMEM, FailedOutOfRangeTheWholeStorage) {
  constexpr auto kStorageSz{512};

  auto storage{mm::make_unique_for_overwrite_bytes(kStorageSz)};

  auto tgt{ublk::inmem::Target{std::move(storage), kStorageSz}};

  auto const buf{mm::make_unique_for_overwrite_bytes(16)};
  auto const buf_span{std::span{buf.get(), 16}};

  auto const res{
      tgt.process(read_query::create(
          buf_span, kStorageSz - buf_span.size() + 1,
          [](read_query const &rq) { EXPECT_EQ(rq.err(), 0); })),
  };
  EXPECT_EQ(res, EINVAL);
}

TEST(INMEM, SuccessfulWritingAtTheWholeStorage) {
  constexpr auto kStorageSz{4_KiB};

  auto storage{ut::make_unique_zeroed_storage(kStorageSz)};
  auto *p_storage{storage.get()};

  auto tgt{ublk::inmem::Target{std::move(storage), kStorageSz}};

  auto const buf{mm::make_unique_randomized_bytes(kStorageSz)};
  auto const buf_span{std::span<std::byte const>{buf.get(), kStorageSz}};

  auto const res{
      tgt.process(write_query::create(
          buf_span, 0, [](write_query const &rq) { EXPECT_EQ(rq.err(), 0); })),
  };
  EXPECT_EQ(res, 0);

  EXPECT_THAT(buf_span, ElementsAreArray(p_storage, kStorageSz));
}

TEST(INMEM, FailedOutOfRangeWritingAtTheWholeStorage) {
  constexpr auto kStorageSz{512uz};

  auto storage{mm::make_unique_for_overwrite_bytes(kStorageSz)};

  auto tgt{ublk::inmem::Target{std::move(storage), kStorageSz}};

  auto const buf{mm::make_unique_for_overwrite_bytes(16uz)};
  auto const buf_span{std::span<std::byte const>{buf.get(), 16uz}};

  auto const res{
      tgt.process(write_query::create(
          buf_span, kStorageSz - buf_span.size() + 1,
          [](write_query const &rq) { EXPECT_EQ(rq.err(), 0); })),
  };
  EXPECT_EQ(res, EINVAL);
}

} // namespace ublk::ut::inmem
