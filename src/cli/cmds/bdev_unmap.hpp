#pragma once

#include <cassert>

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include "args.hpp"
#include "cmd.hpp"

#include "cli/bdev_unmap_param.hpp"
#include "cli/bdev_unmapper_interface.hpp"

namespace ublk::cli::cmds {

class BdevUnmap final : public Cmd {
public:
  explicit BdevUnmap(Args args, std::shared_ptr<IBdevUnmapper> handler)
      : Cmd(std::move(args)), handler_(std::move(handler)) {
    assert(handler_);
  }

  void exec() override {
    bdev_unmap_param param;

    param.bdev_suffix = args_.pop().value_or(std::string{});
    if (param.bdev_suffix.empty())
      throw std::invalid_argument("bdev_suffix cannot be empty");

    handler_->handle(param);
  }

private:
  std::shared_ptr<IBdevUnmapper> handler_;
};

} // namespace ublk::cli::cmds
