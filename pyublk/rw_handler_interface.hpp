#pragma once

#include <cstddef>

#include <bits/types.h>
#include <sys/types.h>

#include <span>

namespace ublk {

class IRWHandler {
public:
  IRWHandler() = default;
  virtual ~IRWHandler() = default;

  IRWHandler(IRWHandler const &) = default;
  IRWHandler &operator=(IRWHandler const &) = default;

  IRWHandler(IRWHandler &&) = default;
  IRWHandler &operator=(IRWHandler &&) = default;

  virtual ssize_t read(std::span<std::byte> buf, __off64_t offset) noexcept = 0;

  virtual ssize_t write(std::span<std::byte const> buf,
                        __off64_t offset) noexcept = 0;
};

} // namespace ublk
