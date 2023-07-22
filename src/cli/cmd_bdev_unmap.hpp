#pragma once

#include <cassert>

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include "cmd.hpp"
#include "cmd_args.hpp"

#include "bdev_unmap_param.hpp"
#include "bdev_unmapper_interface.hpp"

namespace cfq::cli {

class CmdBdevUnmap final : public Cmd {
public:
  explicit CmdBdevUnmap(CmdArgs args, std::shared_ptr<IBdevUnmapper> handler)
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

} // namespace cfq::cli
