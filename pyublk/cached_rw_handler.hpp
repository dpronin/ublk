#pragma once

#include <cstdint>

#include <functional>
#include <memory>
#include <span>
#include <stack>

#include "mm/mem_types.hpp"

#include "flat_lru_cache.hpp"
#include "rw_handler_interface.hpp"
#include "sector.hpp"
#include "span.hpp"

namespace ublk {

class CachedRWHandler : public IRWHandler {
private:
  constexpr static inline auto kCachedChunkAlignment = kSectorSz;
  static_assert(is_aligned_to(kCachedChunkAlignment,
                              alignof(std::max_align_t)));

  template <typename T = std::byte>
  auto mem_chunk_view(mm::uptrwd<std::byte[]> const &mem_chunk) const noexcept {
    return to_span_of<T>({mem_chunk.get(), cache_->item_sz()});
  }

  mm::uptrwd<std::byte[]> mem_chunk_get() noexcept;
  void mem_chunk_put(mm::uptrwd<std::byte[]> &&) noexcept;

public:
  explicit CachedRWHandler(
      std::unique_ptr<flat_lru_cache<uint64_t, std::byte>> cache,
      std::unique_ptr<IRWHandler> handler, bool write_through = true);
  ~CachedRWHandler() override = default;

  CachedRWHandler(CachedRWHandler const &) = delete;
  CachedRWHandler &operator=(CachedRWHandler const &) = delete;

  CachedRWHandler(CachedRWHandler &&) = default;
  CachedRWHandler &operator=(CachedRWHandler &&) = default;

  void set_write_through(bool value) noexcept;
  bool write_through() const noexcept;

  int submit(std::shared_ptr<read_query> rq) noexcept override;
  int submit(std::shared_ptr<write_query> wq) noexcept override;

private:
  std::unique_ptr<flat_lru_cache<uint64_t, std::byte>> cache_;
  std::unique_ptr<IRWHandler> handler_;
  std::function<void(uint64_t chunk_id, std::span<std::byte const> chunk)>
      cache_updater_;
  bool write_through_;
  std::function<mm::uptrwd<std::byte[]>()> mem_chunk_generator_;
  std::stack<mm::uptrwd<std::byte[]>> mem_chunks_;
};

} // namespace ublk
