#pragma once

#include <cassert>

#include <memory>
#include <utility>

#include "cmd_interface.hpp"

namespace ublk::cli {

class CmdQuit final : public ICmd {
public:
  explicit CmdQuit(std::shared_ptr<bool> finish_token)
      : finish_token_(std::move(finish_token)) {
    assert(finish_token_);
  }

  void exec() override { *finish_token_ = true; }

private:
  std::shared_ptr<bool> finish_token_;
};

} // namespace ublk::cli
