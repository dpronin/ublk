#pragma once

#include <cassert>

#include <memory>
#include <utility>

#include "cli/bdev_unmapper_interface.hpp"

#include "unmapper_interface.hpp"

namespace cfq {

class BdevUnmapper : public cli::IBdevUnmapper {
public:
  explicit BdevUnmapper(
      std::shared_ptr<IUnmapper<cli::bdev_unmap_param const &>> unmapper)
      : unmapper_(std::move(unmapper)) {
    assert(unmapper_);
  }
  ~BdevUnmapper() override = default;

  BdevUnmapper(BdevUnmapper const &) = default;
  BdevUnmapper &operator=(BdevUnmapper const &) = default;

  BdevUnmapper(BdevUnmapper &&) = default;
  BdevUnmapper &operator=(BdevUnmapper &&) = default;

  void handle(cli::bdev_unmap_param const &param) override {
    unmapper_->unmap(param);
  }

private:
  std::shared_ptr<IUnmapper<cli::bdev_unmap_param const &>> unmapper_;
};

} // namespace cfq
