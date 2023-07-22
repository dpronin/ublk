#pragma once

#include <cassert>

#include <memory>
#include <utility>

#include "cli/target_destroyer_interface.hpp"

#include "master.hpp"

namespace cfq {

class TargetDestroyer : public cli::ITargetDestroyer {
public:
  explicit TargetDestroyer(std::shared_ptr<Master> master)
      : master_(std::move(master)) {
    assert(master_);
  }
  ~TargetDestroyer() override = default;

  TargetDestroyer(TargetDestroyer const &) = default;
  TargetDestroyer &operator=(TargetDestroyer const &) = default;

  TargetDestroyer(TargetDestroyer &&) = default;
  TargetDestroyer &operator=(TargetDestroyer &&) = default;

  void handle(cli::target_destroy_param const &param) override {
    master_->destroy(param);
  }

private:
  std::shared_ptr<Master> master_;
};

} // namespace cfq
