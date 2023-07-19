#pragma once

#include <cassert>

#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include "cmd.hpp"
#include "cmd_args.hpp"
#include "cmd_interface.hpp"

#include "master.hpp"

namespace cfq {

class CmdBdevDestroy final : public Cmd {
public:
  explicit CmdBdevDestroy(CmdArgs args, std::shared_ptr<Master> master)
      : Cmd(std::move(args)), master_(std::move(master)) {
    assert(master_);
  }

  void exec() override {
    bdev_destroy_param param;

    param.bdev_suffix = args_.pop().value_or(std::string{});
    if (param.bdev_suffix.empty()) {
      std::cerr << "bdev_suffix cannot be empty\n";
      return;
    }

    master_->destroy(param);
  }

private:
  std::shared_ptr<Master> master_;
};

} // namespace cfq
