#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>

#include <algorithm>
#include <memory>
#include <ranges>
#include <vector>

#include "helpers.hpp"

#include "raid0/target.hpp"

namespace ublk::ut::raid0 {

struct target_cfg {
  size_t strip_sz;
  size_t strips_per_stripe_nr;
  size_t stripes_nr;
};

template <typename ParamType>
class Base : public testing::TestWithParam<ParamType> {
protected:
  void SetUp() override {
    auto const &param{this->GetParam()};

    auto const &target_cfg{param.target_cfg};

    backend_ctxs_.resize(target_cfg.strips_per_stripe_nr);
    std::ranges::generate(backend_ctxs_, [] -> backend_ctx {
      return {
          .h = std::make_shared<ut::MockRWHandler>(),
      };
    });

    auto hs{
        std::views::all(backend_ctxs_) |
            std::views::transform([](auto const &hs_ctx) { return hs_ctx.h; }),
    };

    target_ = std::make_unique<ublk::raid0::Target>(target_cfg.strip_sz, hs);
  }

  void TearDown() override {
    target_.reset();
    backend_ctxs_.clear();
  }

  struct backend_ctx {
    std::shared_ptr<ut::MockRWHandler> h;
  };
  std::vector<backend_ctx> backend_ctxs_;

  std::unique_ptr<ublk::raid0::Target> target_;
};

} // namespace ublk::ut::raid0
