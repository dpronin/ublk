#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cerrno>
#include <cstddef>

#include <algorithm>
#include <memory>
#include <span>

#include "mm/mem.hpp"

#include "utils/size_units.hpp"

#include "read_query.hpp"
#include "write_query.hpp"

#include "raid0/backend.hpp"

#include "helpers.hpp"

using namespace ublk;
using namespace testing;

namespace {

class RAID0_BackendFailure : public Test {
protected:
  static constexpr auto kStripSz{4_KiB};
  static constexpr auto kStripsInStripeNr{2};
  static constexpr auto kStripeSz{kStripSz * kStripsInStripeNr};

  void SetUp() override {
    hs_.resize(kStripsInStripeNr);
    std::ranges::generate(
        hs_, [] { return std::make_shared<StrictMock<ut::MockRWHandler>>(); });
    be_ = std::make_unique<ublk::raid0::backend>(kStripSz, hs_);
  }

  void TearDown() override {
    be_.reset();
    hs_.clear();
  }

  std::vector<std::shared_ptr<ut::MockRWHandler>> hs_;
  std::unique_ptr<ublk::raid0::backend> be_;
};

} // namespace

TEST_F(RAID0_BackendFailure, FailureAtFirstStripOfFullStripeRead) {
  auto buf{mm::make_unique_for_overwrite_bytes(this->kStripeSz)};
  auto buf_span{std::span{buf.get(), this->kStripeSz}};

  EXPECT_CALL(*hs_[0], submit(An<std::shared_ptr<read_query>>()))
      .WillOnce(Return(EIO));
  EXPECT_CALL(*hs_[1], submit(An<std::shared_ptr<read_query>>())).Times(0);

  auto const r{be_->process(read_query::create(buf_span, 0))};
  EXPECT_EQ(r, EIO);
}

TEST_F(RAID0_BackendFailure, FailureAtSecondStripOfFullStripeRead) {
  auto buf{mm::make_unique_for_overwrite_bytes(this->kStripeSz)};
  auto buf_span{std::span{buf.get(), this->kStripeSz}};

  EXPECT_CALL(*hs_[0], submit(An<std::shared_ptr<read_query>>()))
      .WillOnce(Return(0));
  EXPECT_CALL(*hs_[1], submit(An<std::shared_ptr<read_query>>()))
      .WillOnce(Return(EIO));

  auto const r{be_->process(read_query::create(buf_span, 0))};
  EXPECT_EQ(r, EIO);
}

TEST_F(RAID0_BackendFailure, FailureAtFirstStripOfFullStripeWrite) {
  auto buf{
      std::unique_ptr<std::byte const[]>{
          mm::make_unique_for_overwrite_bytes(this->kStripeSz),
      },
  };
  auto buf_span{std::span{buf.get(), this->kStripeSz}};

  EXPECT_CALL(*hs_[0], submit(An<std::shared_ptr<write_query>>()))
      .WillOnce(Return(EIO));
  EXPECT_CALL(*hs_[1], submit(An<std::shared_ptr<write_query>>())).Times(0);

  auto const r{be_->process(write_query::create(buf_span, 0))};
  EXPECT_EQ(r, EIO);
}

TEST_F(RAID0_BackendFailure, FailureAtSecondStripOfFullStripeWrite) {
  auto buf{
      std::unique_ptr<std::byte const[]>{
          mm::make_unique_for_overwrite_bytes(this->kStripeSz),
      },
  };
  auto buf_span{std::span{buf.get(), this->kStripeSz}};

  EXPECT_CALL(*hs_[0], submit(An<std::shared_ptr<write_query>>()))
      .WillOnce(Return(0));
  EXPECT_CALL(*hs_[1], submit(An<std::shared_ptr<write_query>>()))
      .WillOnce(Return(EIO));

  auto const r{be_->process(write_query::create(buf_span, 0))};
  EXPECT_EQ(r, EIO);
}
