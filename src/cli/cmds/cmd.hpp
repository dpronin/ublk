#pragma once

#include <utility>

#include "cli/cmds/args.hpp"
#include "cli/cmds/cmd_interface.hpp"
#include "cli/types.hpp"

namespace ublk::cli::cmds {

class Cmd : public ICmd {
protected:
  Args args_;

  explicit Cmd(Args args = {}) : args_(std::move(args)) {}

public:
  ~Cmd() override = default;

  Cmd(Cmd const &) = delete;
  Cmd &operator=(Cmd const &) = delete;

  Cmd(Cmd &&) = delete;
  Cmd &operator=(Cmd &&) = delete;
};

} // namespace ublk::cli::cmds
