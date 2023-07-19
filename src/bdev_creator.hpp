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
  ~BdevCreator() override = default;

  BdevCreator(BdevCreator const &) = default;
  BdevCreator &operator=(BdevCreator const &) = default;

  BdevCreator(BdevCreator &&) = default;
  BdevCreator &operator=(BdevCreator &&) = default;

  void handle(cli::bdev_create_param const &param) override {
    master_->create(param);
  }

private:
  std::shared_ptr<Master> master_;
};

} // namespace cfq
