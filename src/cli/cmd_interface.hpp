#pragma once

namespace ublk::cli {

class ICmd {
public:
  ICmd() = default;
  virtual ~ICmd() = default;

  ICmd(ICmd const &) = default;
  ICmd &operator=(ICmd const &) = default;

  ICmd(ICmd &&) = default;
  ICmd &operator=(ICmd &&) = default;

  virtual void exec() = 0;
};

} // namespace ublk::cli
