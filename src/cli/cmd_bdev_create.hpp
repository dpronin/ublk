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
#include "bdev_creator_interface.hpp"

namespace cfq::cli {

class CmdBdevCreate final : public Cmd {
public:
  explicit CmdBdevCreate(CmdArgs args, std::shared_ptr<IBdevCreator> handler)
      : Cmd(std::move(args)), handler_(std::move(handler)) {
    assert(handler_);
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

    handler_->handle(param);
  }

private:
  std::shared_ptr<IBdevCreator> handler_;
};

} // namespace cfq::cli
