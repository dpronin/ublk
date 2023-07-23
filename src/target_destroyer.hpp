#pragma once

#include <cassert>

#include <memory>
#include <utility>

#include "cli/target_destroyer_interface.hpp"

#include "destroyer_interface.hpp"

namespace ublk {

class TargetDestroyer : public cli::ITargetDestroyer {
public:
  explicit TargetDestroyer(
      std::shared_ptr<IDestroyer<cli::target_destroy_param const &>> destroyer)
      : destroyer_(std::move(destroyer)) {
    assert(destroyer_);
  }
  ~TargetDestroyer() override = default;

  TargetDestroyer(TargetDestroyer const &) = default;
  TargetDestroyer &operator=(TargetDestroyer const &) = default;

  TargetDestroyer(TargetDestroyer &&) = default;
  TargetDestroyer &operator=(TargetDestroyer &&) = default;

  void handle(cli::target_destroy_param const &param) override {
    destroyer_->destroy(param);
  }

private:
  std::shared_ptr<IDestroyer<cli::target_destroy_param const &>> destroyer_;
};

} // namespace ublk
