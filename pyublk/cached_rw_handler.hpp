#pragma once

#include <cstddef>
#include <cstdint>

#include <functional>
#include <memory>
#include <span>

#include "flat_lru_cache.hpp"
#include "mem_types.hpp"
#include "rw_handler_interface.hpp"

namespace ublk {

class CachedRWHandler : public IRWHandler {
public:
  explicit CachedRWHandler(
      std::unique_ptr<flat_lru_cache<uint64_t, std::byte>> cache,
      std::unique_ptr<IRWHandler> handler, bool write_through = true);
  ~CachedRWHandler() override = default;

  CachedRWHandler(CachedRWHandler const &) = delete;
  CachedRWHandler &operator=(CachedRWHandler const &) = delete;

  CachedRWHandler(CachedRWHandler &&) = default;
  CachedRWHandler &operator=(CachedRWHandler &&) = default;

  ssize_t read(std::span<std::byte> buf, __off64_t offset) noexcept override;
  ssize_t write(std::span<std::byte const> buf,
                __off64_t offset) noexcept override;

private:
  std::unique_ptr<flat_lru_cache<uint64_t, std::byte>> cache_;
  std::unique_ptr<IRWHandler> handler_;
  std::function<void(uint64_t chunk_id_at, uint64_t chunk_offset_at,
                     std::span<std::byte const> chunk)>
      cache_updater_;
  uptrwd<std::byte[]> chunk_tmp_;
};

} // namespace ublk
