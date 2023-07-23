#pragma once

#include <cassert>

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include "cmd.hpp"
#include "cmd_args.hpp"

#include "target_destroy_param.hpp"
#include "target_destroyer_interface.hpp"

namespace ublk::cli {

class CmdTargetDestroy final : public Cmd {
public:
  explicit CmdTargetDestroy(CmdArgs args,
                            std::shared_ptr<ITargetDestroyer> handler)
      : Cmd(std::move(args)), handler_(std::move(handler)) {
    assert(handler_);
  }

  void exec() override {
    target_destroy_param param;

    param.name = args_.pop().value_or(std::string{});
    if (param.name.empty())
      throw std::invalid_argument("name cannot be empty");

    handler_->handle(param);
  }

private:
  std::shared_ptr<ITargetDestroyer> handler_;
};

} // namespace ublk::cli
