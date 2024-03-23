#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>

#include <algorithm>
#include <memory>
#include <ranges>
#include <vector>

#include "helpers.hpp"

#include "raid0/backend.hpp"

namespace ublk::ut::raid0 {

struct backend_cfg {
  size_t strip_sz;
  size_t strips_per_stripe_nr;
};

template <typename ParamType>
class Base : public testing::TestWithParam<ParamType> {
protected:
  void SetUp() override {
    auto const &param{this->GetParam()};

    auto const &be_cfg{param.be_cfg};

    backend_ctxs_.resize(be_cfg.strips_per_stripe_nr);
    std::ranges::generate(backend_ctxs_, [] -> backend_ctx {
      return {
          .h = std::make_shared<ut::MockRWHandler>(),
      };
    });

    auto hs{
        std::views::all(backend_ctxs_) |
            std::views::transform([](auto const &hs_ctx) { return hs_ctx.h; }),
    };

    be_ = std::make_unique<ublk::raid0::backend>(be_cfg.strip_sz, hs);
  }

  void TearDown() override {
    be_.reset();
    backend_ctxs_.clear();
  }

  struct backend_ctx {
    std::shared_ptr<ut::MockRWHandler> h;
  };
  std::vector<backend_ctx> backend_ctxs_;

  std::unique_ptr<ublk::raid0::backend> be_;
};

} // namespace ublk::ut::raid0
