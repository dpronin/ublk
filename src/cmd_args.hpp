#pragma once

#include <functional>
#include <optional>
#include <utility>

#include "types.hpp"

namespace cfq {

class CmdArgs {
  args_t args_;

public:
  CmdArgs(args_t args = {}) : args_(std::move(args)) {
    std::erase_if(args_, std::mem_fn(&arg_t::empty));
  }

  std::optional<arg_t> pop() {
    if (!empty()) {
      auto arg = arg_t{std::move(args_.front())};
      args_.pop_front();
      return arg;
    }
    return {};
  }

  [[nodiscard]] bool empty() const noexcept { return args_.empty(); }

  [[nodiscard]] std::optional<arg_t> front() const {
    if (!empty())
      return args_.front();
    return {};
  }
};

} // namespace cfq
