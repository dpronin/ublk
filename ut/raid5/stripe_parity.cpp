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

#include "write_query.hpp"

#include "raid5/target.hpp"

#include "helpers.hpp"

using namespace ublk;
using namespace testing;

namespace {

class RAID5_StripeParity : public Test {
protected:
  constexpr static auto kStripSz{4_KiB};
  constexpr static auto kStripsInStripeNr{2uz};
  constexpr static auto kStripesNr{3uz};
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

TEST_F(RAID5_StripeParity, AllStripesParityIncoherentAtInit) {
  for (auto stripe_id : std::views::iota(0uz, kStripesNr))
    EXPECT_FALSE(target_->is_stripe_parity_coherent(stripe_id));
}

TEST_F(RAID5_StripeParity, StripeParityBecomesCoherentAtEachFullStripeWrite) {
  auto buf{
      std::unique_ptr<std::byte const[]>{
          mm::make_unique_for_overwrite_bytes(this->kStripeDataSz),
      },
  };
  auto buf_span{std::span{buf.get(), this->kStripeDataSz}};

  for (auto const &h : hs_) {
    EXPECT_CALL(*h, submit(Matcher<std::shared_ptr<write_query>>(NotNull())))
        .WillRepeatedly(Return(0));
  }

  for (auto stripe_id : std::views::iota(0uz, kStripesNr)) {
    auto const r{
        target_->process(
            write_query::create(buf_span, stripe_id * this->kStripeDataSz)),
    };
    EXPECT_EQ(r, 0);

    EXPECT_TRUE(target_->is_stripe_parity_coherent(stripe_id));

    for (auto stripe_id_next : std::views::iota(stripe_id + 1, kStripesNr))
      EXPECT_FALSE(target_->is_stripe_parity_coherent(stripe_id_next));
  }
}

TEST_F(RAID5_StripeParity,
       StripeParityBecomesCoherentAtEachFullStripeWriteInReverseOrder) {
  auto buf{
      std::unique_ptr<std::byte const[]>{
          mm::make_unique_for_overwrite_bytes(this->kStripeDataSz),
      },
  };
  auto buf_span{std::span{buf.get(), this->kStripeDataSz}};

  for (auto const &h : hs_) {
    EXPECT_CALL(*h, submit(Matcher<std::shared_ptr<write_query>>(NotNull())))
        .WillRepeatedly(Return(0));
  }

  for (auto stripe_id :
       std::views::iota(0uz, kStripesNr) | std::views::reverse) {
    auto const r{
        target_->process(
            write_query::create(buf_span, stripe_id * this->kStripeDataSz)),
    };
    EXPECT_EQ(r, 0);

    EXPECT_TRUE(target_->is_stripe_parity_coherent(stripe_id));

    for (auto stripe_id_next : std::views::iota(0uz, stripe_id))
      EXPECT_FALSE(target_->is_stripe_parity_coherent(stripe_id_next));
  }
}
