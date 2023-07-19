#pragma once

#include <cassert>

#include <memory>
#include <utility>

#include "cli/bdev_creator_interface.hpp"

#include "master.hpp"

namespace cfq {

class BdevCreator : public cli::IBdevCreator {
public:
  explicit BdevCreator(std::shared_ptr<Master> master)
      : master_(std::move(master)) {
    assert(master_);
  }

  void handle(cli::bdev_create_param const &param) override {
    master_->create(param);
  }

private:
  std::shared_ptr<Master> master_;
};

} // namespace cfq
