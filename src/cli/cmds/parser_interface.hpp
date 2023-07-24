#pragma once

#include <memory>

#include "cmd_interface.hpp"

#include "cli/types.hpp"

namespace ublk::cli::cmds {

class IParser {
public:
  IParser() = default;
  virtual ~IParser() = default;

  IParser(IParser const &) = default;
  IParser &operator=(IParser const &) = default;

  IParser(IParser &&) noexcept = default;
  IParser &operator=(IParser &&) noexcept = default;

  [[nodiscard]] virtual std::unique_ptr<ICmd> parse(args_t args) = 0;
};

} // namespace ublk::cli::cmds
