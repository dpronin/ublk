#pragma once

#include "cmd_parser_interface.hpp"
#include "types.hpp"

namespace cfq {

class CmdParserDefault : public ICmdParser {
protected:
  cmds_t cmds_;

public:
  explicit CmdParserDefault(cmds_t cmds = {});
  ~CmdParserDefault() override = default;

  CmdParserDefault(CmdParserDefault const &) = delete;
  CmdParserDefault &operator=(CmdParserDefault const &) = delete;

  CmdParserDefault(CmdParserDefault &&) = default;
  CmdParserDefault &operator=(CmdParserDefault &&) = default;

  [[nodiscard]] std::unique_ptr<ICmd> parse(args_t args) override;
};

} // namespace cfq
