#pragma once

#include <cassert>

#include <memory>
#include <utility>

#include "cli/bdev_mapper_interface.hpp"

#include "mapper_interface.hpp"

namespace ublk {

class BdevMapper : public cli::IBdevMapper {
public:
  explicit BdevMapper(
      std::shared_ptr<IMapper<cli::bdev_map_param const &>> mapper)
      : mapper_(std::move(mapper)) {
    assert(mapper_);
  }
  ~BdevMapper() override = default;

  BdevMapper(BdevMapper const &) = default;
  BdevMapper &operator=(BdevMapper const &) = default;

  BdevMapper(BdevMapper &&) = default;
  BdevMapper &operator=(BdevMapper &&) = default;

  void handle(cli::bdev_map_param const &param) override {
    mapper_->map(param);
  }

private:
  std::shared_ptr<IMapper<cli::bdev_map_param const &>> mapper_;
};

} // namespace ublk
