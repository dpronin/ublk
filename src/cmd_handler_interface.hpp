#pragma once

#include <linux/ublk/cmd.h>

namespace cfq {

template <typename T> class ICmdHandler {
public:
  ICmdHandler() = default;
  virtual ~ICmdHandler() = default;

  ICmdHandler(ICmdHandler const &) = default;
  ICmdHandler &operator=(ICmdHandler const &) = default;

  ICmdHandler(ICmdHandler &&) = default;
  ICmdHandler &operator=(ICmdHandler &&) = default;

  virtual int handle(T &cmd) noexcept = 0;
};

} // namespace cfq
