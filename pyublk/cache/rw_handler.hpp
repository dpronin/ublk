#pragma once

#include <cstddef>
#include <cstdint>

#include <array>
#include <memory>

#include "utils/utility.hpp"

#include "rw_handler_interface.hpp"
#include "sector.hpp"

namespace ublk::cache {

class RWHandler : public IRWHandler {
private:
  constexpr static auto kCachedChunkAlignment = kSectorSz;
  static_assert(is_aligned_to(kCachedChunkAlignment,
                              alignof(std::max_align_t)));

public:
  explicit RWHandler(uint64_t cache_len_sectors,
                     std::unique_ptr<IRWHandler> handler,
                     bool write_through = true);
  ~RWHandler() override = default;

  RWHandler(RWHandler const &) = delete;
  RWHandler &operator=(RWHandler const &) = delete;

  RWHandler(RWHandler &&) = delete;
  RWHandler &operator=(RWHandler &&) = delete;

  void set_write_through(bool value) noexcept;
  bool write_through() const noexcept;

  int submit(std::shared_ptr<read_query> rq) noexcept override;
  int submit(std::shared_ptr<write_query> wq) noexcept override;

private:
  std::shared_ptr<IRWHandler> handler_;
  std::array<std::shared_ptr<IRWHandler>, 2> handlers_;
};

} // namespace ublk::cache
