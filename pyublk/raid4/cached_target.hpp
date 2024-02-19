#pragma once

#include <cstddef>
#include <cstdint>

#include <memory>
#include <span>
#include <vector>

#include "flat_lru_cache.hpp"
#include "rw_handler_interface.hpp"
#include "target.hpp"

namespace ublk::raid4 {

class CachedTarget : public Target {
public:
  explicit CachedTarget(
      uint64_t strip_sz,
      std::unique_ptr<flat_lru_cache<uint64_t, std::byte>> cache,
      std::vector<std::shared_ptr<IRWHandler>> hs);
  ~CachedTarget() override = default;

  CachedTarget(CachedTarget const &) = delete;
  CachedTarget &operator=(CachedTarget const &) = delete;

  CachedTarget(CachedTarget &&) = default;
  CachedTarget &operator=(CachedTarget &&) = default;

  ssize_t read(std::span<std::byte> buf, __off64_t offset) noexcept override;
  ssize_t write(std::span<std::byte const> buf,
                __off64_t offset) noexcept override;

private:
  std::unique_ptr<flat_lru_cache<uint64_t, std::byte>> cache_;
};

} // namespace ublk::raid4
