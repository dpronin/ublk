#pragma once

#include <cstddef>

#include <linux/types.h>

#include <span>

namespace cfq {

class IWriteHandler {
public:
  IWriteHandler() = default;
  virtual ~IWriteHandler() = default;

  IWriteHandler(IWriteHandler const &) = default;
  IWriteHandler &operator=(IWriteHandler const &) = default;

  IWriteHandler(IWriteHandler &&) = default;
  IWriteHandler &operator=(IWriteHandler &&) = default;

  virtual int handle(std::span<std::byte const> buf,
                     __off64_t offset) noexcept = 0;
};

} // namespace cfq
