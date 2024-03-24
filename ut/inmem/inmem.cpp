#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>

#include "utils/size_units.hpp"

#include "helpers.hpp"

#include "inmem/target.hpp"

#include "read_query.hpp"
#include "write_query.hpp"

using namespace testing;

namespace ublk::ut::inmem {

TEST(INMEM, TestReading) {
  constexpr auto kStorageSz{4_KiB};

  auto storage{ut::make_unique_randomized_storage<std::byte>(kStorageSz)};
  auto const storage_span{std::span{storage.get(), kStorageSz}};

  auto tgt{ublk::inmem::Target{std::move(storage), kStorageSz}};

  auto const buf{mm::make_unique_zeroed_bytes(kStorageSz)};
  auto const buf_span{std::span{buf.get(), kStorageSz}};

  auto const res{
      tgt.process(read_query::create(
          buf_span, 0, [](read_query const &rq) { EXPECT_EQ(rq.err(), 0); })),
  };
  EXPECT_EQ(res, 0);

  EXPECT_THAT(buf_span, ElementsAreArray(storage_span));
}

TEST(INMEM, TestWriting) {
  constexpr auto kStorageSz{4_KiB};

  auto storage{ut::make_unique_zeroed_storage<std::byte>(kStorageSz)};
  auto const storage_span{std::span{storage.get(), kStorageSz}};

  auto tgt{ublk::inmem::Target{std::move(storage), kStorageSz}};

  auto const buf{
      std::unique_ptr<std::byte const[]>{
          mm::make_unique_randomized_bytes(kStorageSz)},
  };
  auto const buf_span{std::span{buf.get(), kStorageSz}};

  auto const res{
      tgt.process(write_query::create(
          buf_span, 0, [](write_query const &rq) { EXPECT_EQ(rq.err(), 0); })),
  };
  EXPECT_EQ(res, 0);

  EXPECT_THAT(buf_span, ElementsAreArray(storage_span));
}

} // namespace ublk::ut::inmem
