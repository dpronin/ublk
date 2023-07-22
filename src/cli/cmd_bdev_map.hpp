#pragma once

#include <cassert>
#include <cstdint>

#include <iostream>
#include <memory>
#include <utility>

#include <boost/lexical_cast.hpp>
#include <fmt/format.h>

#include "cmd.hpp"
#include "cmd_args.hpp"
#include "cmd_interface.hpp"

#include "bdev_map_param.hpp"
#include "bdev_mapper_interface.hpp"

namespace cfq::cli {

class CmdBdevMap final : public Cmd {
public:
  explicit CmdBdevMap(CmdArgs args, std::shared_ptr<IBdevMapper> handler)
      : Cmd(std::move(args)), handler_(std::move(handler)) {
    assert(handler_);
  }

  void exec() override {
    bdev_map_param param;

    param.bdev_suffix = args_.pop().value_or(std::string{});
    if (param.bdev_suffix.empty())
      throw std::invalid_argument("bdev_suffix cannot be empty");

    param.target_name = args_.pop().value_or(std::string{});
    if (param.target_name.empty())
      throw std::invalid_argument(fmt::format(
          "target-name '{}' does not have a filename", param.target_name));

    param.read_only =
        boost::lexical_cast<bool>(args_.pop().value_or(std::string{"0"}));

    handler_->handle(param);
  }

private:
  std::shared_ptr<IBdevMapper> handler_;
};

} // namespace cfq::cli
