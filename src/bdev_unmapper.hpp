#pragma once

#include <cassert>

#include <memory>
#include <utility>

#include "cli/bdev_unmapper_interface.hpp"

#include "master.hpp"

namespace cfq {

class BdevUnmapper : public cli::IBdevUnmapper {
public:
  explicit BdevUnmapper(std::shared_ptr<Master> master)
      : master_(std::move(master)) {
    assert(master_);
  }
  ~BdevUnmapper() override = default;

  BdevUnmapper(BdevUnmapper const &) = default;
  BdevUnmapper &operator=(BdevUnmapper const &) = default;

  BdevUnmapper(BdevUnmapper &&) = default;
  BdevUnmapper &operator=(BdevUnmapper &&) = default;

  void handle(cli::bdev_unmap_param const &param) override {
    master_->unmap(param);
  }

private:
  std::shared_ptr<Master> master_;
};

} // namespace cfq
