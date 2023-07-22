#pragma once

#include <cassert>

#include <memory>
#include <utility>

#include "cli/target_creator_interface.hpp"

#include "master.hpp"

namespace cfq {

class TargetCreator : public cli::ITargetCreator {
public:
  explicit TargetCreator(std::shared_ptr<Master> master)
      : master_(std::move(master)) {
    assert(master_);
  }
  ~TargetCreator() override = default;

  TargetCreator(TargetCreator const &) = default;
  TargetCreator &operator=(TargetCreator const &) = default;

  TargetCreator(TargetCreator &&) = default;
  TargetCreator &operator=(TargetCreator &&) = default;

  void handle(cli::target_create_param const &param) override {
    master_->create(param);
  }

private:
  std::shared_ptr<Master> master_;
};

} // namespace cfq
