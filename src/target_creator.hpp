#pragma once

#include <cassert>

#include <memory>
#include <utility>

#include "cli/target_creator_interface.hpp"

#include "creator_interface.hpp"

namespace ublk {

class TargetCreator : public cli::ITargetCreator {
public:
  explicit TargetCreator(
      std::shared_ptr<ICreator<cli::target_create_param const &>> creator)
      : creator_(std::move(creator)) {
    assert(creator_);
  }
  ~TargetCreator() override = default;

  TargetCreator(TargetCreator const &) = default;
  TargetCreator &operator=(TargetCreator const &) = default;

  TargetCreator(TargetCreator &&) = default;
  TargetCreator &operator=(TargetCreator &&) = default;

  void handle(cli::target_create_param const &param) override {
    creator_->create(param);
  }

private:
  std::shared_ptr<ICreator<cli::target_create_param const &>> creator_;
};

} // namespace ublk
