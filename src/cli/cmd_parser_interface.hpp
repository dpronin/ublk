#pragma once

#include <memory>

#include "cmd_interface.hpp"

#include "types.hpp"

namespace cfq::cli {

class ICmdParser {
public:
  ICmdParser() = default;
  virtual ~ICmdParser() = default;

  ICmdParser(ICmdParser const &) = default;
  ICmdParser &operator=(ICmdParser const &) = default;

  ICmdParser(ICmdParser &&) noexcept = default;
  ICmdParser &operator=(ICmdParser &&) noexcept = default;

  [[nodiscard]] virtual std::unique_ptr<ICmd> parse(args_t args) = 0;
};

} // namespace cfq::cli
