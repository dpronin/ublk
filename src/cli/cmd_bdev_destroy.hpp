#pragma once

#include <cassert>

#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include "cmd.hpp"
#include "cmd_args.hpp"
#include "cmd_interface.hpp"

#include "bdev_destroy_param.hpp"
#include "bdev_destroyer_interface.hpp"

namespace cfq::cli {

class CmdBdevDestroy final : public Cmd {
public:
  explicit CmdBdevDestroy(CmdArgs args, std::shared_ptr<IBdevDestroyer> handler)
      : Cmd(std::move(args)), handler_(std::move(handler)) {
    assert(handler_);
  }

  void exec() override {
    bdev_destroy_param param;

    param.bdev_suffix = args_.pop().value_or(std::string{});
    if (param.bdev_suffix.empty()) {
      std::cerr << "bdev_suffix cannot be empty\n";
      return;
    }

    handler_->handle(param);
  }

private:
  std::shared_ptr<IBdevDestroyer> handler_;
};

} // namespace cfq::cli
