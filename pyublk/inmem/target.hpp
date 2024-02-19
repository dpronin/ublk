#pragma once

#include <cstddef>
#include <cstdint>

#include <bits/types.h>
#include <sys/types.h>

#include <span>

#include "mem_types.hpp"

namespace ublk::inmem {

class Target {
public:
  explicit Target(uptrwd<std::byte[]> mem, uint64_t sz) noexcept;
  ~Target() = default;

  Target(Target const &) = delete;
  Target &operator=(Target const &) = delete;

  Target(Target &&) = default;
  Target &operator=(Target &&) = default;

  ssize_t read(std::span<std::byte> buf, __off64_t offset) noexcept;
  ssize_t write(std::span<std::byte const> buf, __off64_t offset) noexcept;

private:
  uptrwd<std::byte[]> mem_;
  uint64_t mem_sz_;
};

} // namespace ublk::inmem
