#pragma once

#include "cli/types.hpp"

#include "parser_interface.hpp"

namespace ublk::cli::cmds {

class ParserDefault : public IParser {
protected:
  cmds_t cmds_;

public:
  explicit ParserDefault(cmds_t cmds = {});
  ~ParserDefault() override = default;

  ParserDefault(ParserDefault const &) = delete;
  ParserDefault &operator=(ParserDefault const &) = delete;

  ParserDefault(ParserDefault &&) = default;
  ParserDefault &operator=(ParserDefault &&) = default;

  [[nodiscard]] std::unique_ptr<ICmd> parse(args_t args) override;
};

} // namespace ublk::cli::cmds
