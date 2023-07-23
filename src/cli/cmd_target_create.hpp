#pragma once

#include <cassert>
#include <cstdint>

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include <boost/lexical_cast.hpp>
#include <fmt/format.h>

#include "cmd.hpp"
#include "cmd_args.hpp"

#include "target_create_param.hpp"
#include "target_creator_interface.hpp"

namespace ublk::cli {

class CmdTargetCreate final : public Cmd {
public:
  explicit CmdTargetCreate(CmdArgs args,
                           std::shared_ptr<ITargetCreator> handler)
      : Cmd(std::move(args)), handler_(std::move(handler)) {
    assert(handler_);
  }

  void exec() override {
    target_create_param param;

    param.name = args_.pop().value_or(std::string{});
    if (param.name.empty())
      throw std::invalid_argument("name cannot be empty");

    param.path = args_.pop().value_or(std::filesystem::path{});
    if (!param.path.has_filename()) {
      throw std::invalid_argument(fmt::format(
          "path '{}' does not have a filename", param.path.string()));
    }

    param.capacity_sectors =
        boost::lexical_cast<uint64_t>(args_.pop().value_or(std::string{"0"}));
    if (!param.capacity_sectors)
      throw std::invalid_argument("capacity_sectors cannot be 0");

    handler_->handle(param);
  }

private:
  std::shared_ptr<ITargetCreator> handler_;
};

} // namespace ublk::cli
