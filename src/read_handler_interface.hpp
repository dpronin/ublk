#pragma once

#include <cstddef>

#include <linux/types.h>

#include <span>

namespace cfq {

class IReadHandler {
public:
  IReadHandler() = default;
  virtual ~IReadHandler() = default;

  IReadHandler(IReadHandler const &) = default;
  IReadHandler &operator=(IReadHandler const &) = default;

  IReadHandler(IReadHandler &&) = default;
  IReadHandler &operator=(IReadHandler &&) = default;

  virtual int handle(std::span<std::byte> buf, __off64_t offset) noexcept = 0;
};

} // namespace cfq
