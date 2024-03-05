#pragma once

#include <cstddef>
#include <cstdint>

#include <memory>

#include "flat_lru_cache.hpp"
#include "rw_handler_interface.hpp"
#include "sector.hpp"

namespace ublk {

class CachedRWHandler : public IRWHandler {
private:
  constexpr static inline auto kCachedChunkAlignment = kSectorSz;
  static_assert(is_aligned_to(kCachedChunkAlignment,
                              alignof(std::max_align_t)));

public:
  explicit CachedRWHandler(
      std::unique_ptr<flat_lru_cache<uint64_t, std::byte>> cache,
      std::unique_ptr<IRWHandler> handler, bool write_through = true);
  ~CachedRWHandler() override = default;

  CachedRWHandler(CachedRWHandler const &) = delete;
  CachedRWHandler &operator=(CachedRWHandler const &) = delete;

  CachedRWHandler(CachedRWHandler &&) = delete;
  CachedRWHandler &operator=(CachedRWHandler &&) = delete;

  void set_write_through(bool value) noexcept;
  bool write_through() const noexcept;

  int submit(std::shared_ptr<read_query> rq) noexcept override;
  int submit(std::shared_ptr<write_query> wq) noexcept override;

private:
  std::shared_ptr<IRWHandler> handler_;
  std::array<std::shared_ptr<IRWHandler>, 2> handlers_;
};

} // namespace ublk
