#pragma once

#include <cstdint>

#include <memory>
#include <vector>

#include "rw_handler_interface.hpp"

namespace ublk::raid0 {

class Target {
public:
  explicit Target(uint64_t strip_sz,
                  std::vector<std::shared_ptr<IRWHandler>> hs);
  virtual ~Target() = default;

  Target(Target const &) = default;
  Target &operator=(Target const &) = default;

  Target(Target &&) = default;
  Target &operator=(Target &&) = default;

  virtual ssize_t read(std::span<std::byte> buf, __off64_t offset) noexcept;
  virtual ssize_t write(std::span<std::byte const> buf,
                        __off64_t offset) noexcept;

protected:
  ssize_t read(uint64_t strip_id_from, uint64_t strip_offset_from,
               std::span<std::byte> buf) noexcept;
  ssize_t write(uint64_t strip_id_from, uint64_t strip_offset_from,
                std::span<std::byte const> buf) noexcept;

  uint64_t strip_sz_;
  std::vector<std::shared_ptr<IRWHandler>> hs_;
};

} // namespace ublk::raid0
