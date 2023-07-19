#pragma once

#include <utility>

#include "cmd_args.hpp"
#include "cmd_interface.hpp"
#include "types.hpp"

namespace cfq::cli {

class Cmd : public ICmd {
protected:
  CmdArgs args_;

  explicit Cmd(CmdArgs args = {}) : args_(std::move(args)) {}

public:
  ~Cmd() override = default;

  Cmd(Cmd const &) = delete;
  Cmd &operator=(Cmd const &) = delete;

  Cmd(Cmd &&) = delete;
  Cmd &operator=(Cmd &&) = delete;
};

} // namespace cfq::cli
