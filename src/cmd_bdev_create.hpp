#pragma once

#include <cassert>

#include <iostream>
#include <memory>
#include <utility>

#include <fmt/format.h>

#include "cmd.hpp"
#include "cmd_args.hpp"
#include "cmd_interface.hpp"

#include "bdev_create_param.hpp"
#include "master.hpp"

namespace cfq {

class CmdBdevCreate final : public Cmd {
public:
  explicit CmdBdevCreate(CmdArgs args, std::shared_ptr<Master> master)
      : Cmd(std::move(args)), master_(std::move(master)) {
    assert(master_);
  }

  void exec() override {
    bdev_create_param param;

    param.bdev_suffix = args_.pop().value_or(std::string{});
    if (param.bdev_suffix.empty()) {
      std::cerr << "bdev_suffix cannot be empty\n";
      return;
    }

    param.target = args_.pop().value_or(std::filesystem::path{});
    if (!param.target.has_filename()) {
      std::cerr << fmt::format("target '{}' does not have a filename\n",
                               param.target.string());
      return;
    }

    master_->create(param);
  }

private:
  std::shared_ptr<Master> master_;
};

} // namespace cfq
