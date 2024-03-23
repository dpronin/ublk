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

#include "raid0/target.hpp"

#include "helpers.hpp"

using namespace ublk;
using namespace testing;

namespace {

class RAID0 : public Test {
protected:
  static constexpr auto kStripSz{4_KiB};
  static constexpr auto kStripsInStripeNr{2};
  static constexpr auto kStripeSz{kStripSz * kStripsInStripeNr};

  void SetUp() override {
    hs_.resize(kStripsInStripeNr);
    std::ranges::generate(hs_,
                          [] { return std::make_shared<ut::MockRWHandler>(); });
    target_ = std::make_unique<ublk::raid0::Target>(kStripSz, hs_);
  }

  void TearDown() override {
    target_.reset();
    hs_.clear();
  }

  std::vector<std::shared_ptr<ut::MockRWHandler>> hs_;
  std::unique_ptr<ublk::raid0::Target> target_;
};

} // namespace

TEST_F(RAID0, OfflineDueToBackendFailureAtSubmitRead) {
  auto buf{mm::make_unique_for_overwrite_bytes(this->kStripeSz)};
  auto buf_span{std::span{buf.get(), this->kStripeSz}};

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

TEST_F(RAID0, OfflineDueToBackendFailureAtCompleteRead) {
  auto buf{mm::make_unique_for_overwrite_bytes(this->kStripeSz)};
  auto buf_span{std::span{buf.get(), this->kStripeSz}};

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

TEST_F(RAID0, OfflineDueToBackendFailureAtSubmitWrite) {
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

TEST_F(RAID0, OfflineDueToBackendFailureAtCompleteWrite) {
  auto buf{
      std::unique_ptr<std::byte const[]>{
          mm::make_unique_for_overwrite_bytes(this->kStripeSz),
      },
  };
  auto buf_span{std::span{buf.get(), this->kStripeSz}};

  EXPECT_CALL(*hs_[0], submit(An<std::shared_ptr<write_query>>()))
      .WillOnce(Return(0));
  EXPECT_CALL(*hs_[1], submit(An<std::shared_ptr<write_query>>()))
      .WillOnce([](std::shared_ptr<write_query> wq) {
        EXPECT_TRUE(wq);
        wq->set_err(EIO);
        return 0;
      });

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