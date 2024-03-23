#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cerrno>
#include <cstddef>

#include <algorithm>
#include <memory>
#include <ranges>
#include <span>

#include "mm/mem.hpp"

#include "utils/size_units.hpp"

#include "read_query.hpp"
#include "write_query.hpp"

#include "raid5/target.hpp"

#include "helpers.hpp"

using namespace ublk;
using namespace testing;

namespace {

class RAID5_OnlineToOfflineTransition : public Test {
protected:
  constexpr static auto kStripSz{4_KiB};
  constexpr static auto kStripsInStripeNr{2};
  constexpr static auto kStripeDataSz{kStripSz * kStripsInStripeNr};

  void SetUp() override {
    hs_.resize(kStripsInStripeNr + 1);
    std::ranges::generate(
        hs_, [] { return std::make_shared<StrictMock<ut::MockRWHandler>>(); });
    target_ = std::make_unique<ublk::raid5::Target>(kStripSz, hs_);
  }

  void TearDown() override {
    target_.reset();
    hs_.clear();
  }

  std::vector<std::shared_ptr<ut::MockRWHandler>> hs_;
  std::unique_ptr<ublk::raid5::Target> target_;
};

} // namespace

TEST_F(RAID5_OnlineToOfflineTransition,
       GoToOfflineDueToBackendFailureAtSubmitRead) {
  auto buf{mm::make_unique_for_overwrite_bytes(this->kStripeDataSz)};
  auto buf_span{std::span{buf.get(), this->kStripeDataSz}};

  EXPECT_CALL(*hs_[0], submit(An<std::shared_ptr<read_query>>()))
      .WillOnce(Return(0));
  EXPECT_CALL(*hs_[1], submit(An<std::shared_ptr<read_query>>()))
      .WillOnce(Return(EIO));

  auto const r1{
      target_->process(read_query::create(
          buf_span, 0, [](read_query const &rq) { EXPECT_EQ(rq.err(), 0); })),
  };
  EXPECT_EQ(r1, EIO);

  EXPECT_STRCASEEQ(target_->state().c_str(), "offline");

  auto const r2{
      target_->process(read_query::create(
          buf_span.subspan(0, this->kStripSz), 0,
          [](read_query const &rq) { EXPECT_EQ(rq.err(), 0); })),
  };
  EXPECT_EQ(r2, EIO);
}

TEST_F(RAID5_OnlineToOfflineTransition,
       GoToOfflineDueToBackendFailureAtCompleteRead) {
  auto buf{mm::make_unique_for_overwrite_bytes(this->kStripeDataSz)};
  auto buf_span{std::span{buf.get(), this->kStripeDataSz}};

  EXPECT_CALL(*hs_[0], submit(An<std::shared_ptr<read_query>>()))
      .WillOnce(Return(0));
  EXPECT_CALL(*hs_[1], submit(An<std::shared_ptr<read_query>>()))
      .WillOnce([](std::shared_ptr<read_query> rq) {
        EXPECT_TRUE(rq);
        rq->set_err(EIO);
        return 0;
      });

  auto const r1{
      target_->process(read_query::create(
          buf_span, 0, [](read_query const &rq) { EXPECT_EQ(rq.err(), EIO); })),
  };
  EXPECT_EQ(r1, 0);

  EXPECT_STRCASEEQ(target_->state().c_str(), "offline");

  auto const r2{
      target_->process(read_query::create(
          buf_span.subspan(0, this->kStripSz), 0,
          [](read_query const &rq) { EXPECT_EQ(rq.err(), 0); })),
  };
  EXPECT_EQ(r2, EIO);
}

TEST_F(RAID5_OnlineToOfflineTransition,
       GoToOfflineDueToBackendFailureAtSubmitWrite) {
  auto buf{
      std::unique_ptr<std::byte const[]>{
          mm::make_unique_for_overwrite_bytes(this->kStripeDataSz),
      },
  };
  auto buf_span{std::span{buf.get(), this->kStripeDataSz}};

  EXPECT_CALL(*hs_[0], submit(An<std::shared_ptr<write_query>>()))
      .WillOnce(Return(0));
  EXPECT_CALL(*hs_[1], submit(An<std::shared_ptr<write_query>>()))
      .WillOnce(Return(EIO));

  auto const r1{
      target_->process(write_query::create(
          buf_span, 0, [](write_query const &wq) { EXPECT_EQ(wq.err(), 0); })),
  };
  EXPECT_EQ(r1, EIO);

  EXPECT_STRCASEEQ(target_->state().c_str(), "offline");

  auto const r2{
      target_->process(write_query::create(
          buf_span.subspan(0, this->kStripSz), 0,
          [](write_query const &wq) { EXPECT_EQ(wq.err(), 0); })),
  };
  EXPECT_EQ(r2, EIO);
}

TEST_F(RAID5_OnlineToOfflineTransition,
       GoToOfflineDueToBackendFailureAtCompleteWrite) {
  auto buf{
      std::unique_ptr<std::byte const[]>{
          mm::make_unique_for_overwrite_bytes(this->kStripeDataSz),
      },
  };
  auto buf_span{std::span{buf.get(), this->kStripeDataSz}};

  EXPECT_CALL(*hs_[0], submit(An<std::shared_ptr<write_query>>()))
      .WillOnce(Return(0));
  EXPECT_CALL(*hs_[1], submit(An<std::shared_ptr<write_query>>()))
      .WillOnce([](std::shared_ptr<write_query> wq) {
        EXPECT_TRUE(wq);
        wq->set_err(EIO);
        return 0;
      });
  EXPECT_CALL(*hs_[2], submit(An<std::shared_ptr<write_query>>()))
      .WillOnce(Return(0));

  auto const r1{
      target_->process(write_query::create(
          buf_span, 0,
          [](write_query const &wq) { EXPECT_EQ(wq.err(), EIO); })),
  };
  EXPECT_EQ(r1, 0);

  EXPECT_STRCASEEQ(target_->state().c_str(), "offline");

  auto const r2{
      target_->process(write_query::create(
          buf_span.subspan(0, this->kStripSz), 0,
          [](write_query const &wq) { EXPECT_EQ(wq.err(), 0); })),
  };
  EXPECT_EQ(r2, EIO);
}
