#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cerrno>
#include <cstddef>

#include <algorithm>
#include <memory>
#include <span>
#include <vector>

#include "mm/mem.hpp"

#include "utils/size_units.hpp"

#include "read_query.hpp"
#include "write_query.hpp"

#include "raid1/backend.hpp"

#include "helpers.hpp"

using namespace ublk;
using namespace testing;

namespace {

class RAID1_BackendFailure : public Test {
protected:
  constexpr static auto kReadStripSz{4_KiB};
  constexpr static auto kMirrorsNr{2};
  constexpr static auto kReadStripeSz{kReadStripSz * kMirrorsNr};
  constexpr static auto kWriteSz{12_KiB};

  void SetUp() override {
    hs_.resize(kMirrorsNr);
    std::ranges::generate(
        hs_, [] { return std::make_shared<StrictMock<ut::MockRWHandler>>(); });
    be_ = std::make_unique<ublk::raid1::backend>(kReadStripSz, hs_);
  }

  void TearDown() override {
    be_.reset();
    hs_.clear();
  }

  std::vector<std::shared_ptr<ut::MockRWHandler>> hs_;
  std::unique_ptr<ublk::raid1::backend> be_;
};

} // namespace

TEST_F(RAID1_BackendFailure, FailureAtFirstStripOfFullStripeRead) {
  auto buf{mm::make_unique_for_overwrite_bytes(this->kReadStripeSz)};
  auto buf_span{std::span{buf.get(), this->kReadStripeSz}};

  EXPECT_CALL(*hs_[0], submit(An<std::shared_ptr<read_query>>()))
      .WillOnce(Return(EIO));

  auto const r{be_->process(read_query::create(buf_span, 0))};
  EXPECT_EQ(r, EIO);
}

TEST_F(RAID1_BackendFailure, FailureAtSecondStripOfFullStripeRead) {
  auto buf{mm::make_unique_for_overwrite_bytes(this->kReadStripeSz)};
  auto buf_span{std::span{buf.get(), this->kReadStripeSz}};

  EXPECT_CALL(*hs_[0], submit(An<std::shared_ptr<read_query>>()))
      .WillOnce(Return(0));
  EXPECT_CALL(*hs_[1], submit(An<std::shared_ptr<read_query>>()))
      .WillOnce(Return(EIO));

  auto const r{be_->process(read_query::create(buf_span, 0))};
  EXPECT_EQ(r, EIO);
}

TEST_F(RAID1_BackendFailure, FailureAtFirstMirrorWrite) {
  auto buf{
      std::unique_ptr<std::byte const[]>{
          mm::make_unique_for_overwrite_bytes(this->kWriteSz),
      },
  };
  auto buf_span{std::span{buf.get(), this->kWriteSz}};

  EXPECT_CALL(*hs_[0], submit(An<std::shared_ptr<write_query>>()))
      .WillOnce(Return(EIO));

  auto const r{be_->process(write_query::create(buf_span, 0))};
  EXPECT_EQ(r, EIO);
}

TEST_F(RAID1_BackendFailure, FailureAtSecondMirrorWrite) {
  auto buf{
      std::unique_ptr<std::byte const[]>{
          mm::make_unique_for_overwrite_bytes(this->kWriteSz),
      },
  };
  auto buf_span{std::span{buf.get(), this->kWriteSz}};

  EXPECT_CALL(*hs_[0], submit(An<std::shared_ptr<write_query>>()))
      .WillOnce(Return(0));
  EXPECT_CALL(*hs_[1], submit(An<std::shared_ptr<write_query>>()))
      .WillOnce(Return(EIO));

  auto const r{be_->process(write_query::create(buf_span, 0))};
  EXPECT_EQ(r, EIO);
}
