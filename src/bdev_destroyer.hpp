#pragma once

#include <cassert>

#include <memory>
#include <utility>

#include "cli/bdev_destroyer_interface.hpp"

#include "master.hpp"

namespace cfq {

class BdevDestroyer : public cli::IBdevDestroyer {
public:
  explicit BdevDestroyer(std::shared_ptr<Master> master)
      : master_(std::move(master)) {
    assert(master_);
  }

  void handle(cli::bdev_destroy_param const &param) override {
    master_->destroy(param);
  }

private:
  std::shared_ptr<Master> master_;
};

} // namespace cfq
