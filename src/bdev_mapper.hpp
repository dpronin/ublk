#pragma once

#include <cassert>

#include <memory>
#include <utility>

#include "cli/bdev_mapper_interface.hpp"

#include "master.hpp"

namespace cfq {

class BdevMapper : public cli::IBdevMapper {
public:
  explicit BdevMapper(std::shared_ptr<Master> master)
      : master_(std::move(master)) {
    assert(master_);
  }
  ~BdevMapper() override = default;

  BdevMapper(BdevMapper const &) = default;
  BdevMapper &operator=(BdevMapper const &) = default;

  BdevMapper(BdevMapper &&) = default;
  BdevMapper &operator=(BdevMapper &&) = default;

  void handle(cli::bdev_map_param const &param) override {
    master_->map(param);
  }

private:
  std::shared_ptr<Master> master_;
};

} // namespace cfq
