#pragma once

#include <cstddef>
#include <cstdint>

#include <memory>
#include <utility>
#include <vector>

#include <boost/dynamic_bitset/dynamic_bitset.hpp>

#include "utils/bitset_locker.hpp"
#include "utils/span.hpp"

#include "mm/cache_line_aligned_allocator.hpp"
#include "mm/mem_chunk_pool.hpp"

#include "read_query.hpp"
#include "rw_handler_interface.hpp"
#include "sector.hpp"
#include "write_query.hpp"

namespace ublk::raidsp {

class Target {
private:
  std::vector<std::shared_ptr<IRWHandler>>
  stripe_id_to_handlers(uint64_t stripe_id);

  int full_stripe_write_process(uint64_t stripe_id_at,
                                std::shared_ptr<write_query> wq) noexcept;

protected:
  constexpr static inline auto kCachedStripeAlignment = kSectorSz;
  static_assert(is_aligned_to(kCachedStripeAlignment,
                              alignof(std::max_align_t)));

  constexpr static inline auto kCachedParityAlignment = kSectorSz;
  static_assert(is_aligned_to(kCachedParityAlignment,
                              alignof(std::max_align_t)));

  template <typename T = std::byte>
  auto stripe_view(mm::uptrwd<std::byte[]> const &stripe) const noexcept {
    return to_span_of<T>(stripe_pool_->chunk_view(stripe));
  }

  template <typename T = std::byte>
  auto stripe_data_view(mm::uptrwd<std::byte[]> const &stripe) const noexcept {
    return to_span_of<T>(stripe_view(stripe).subspan(0, stripe_data_sz_));
  }

  template <typename T = std::byte>
  auto
  stripe_parity_view(mm::uptrwd<std::byte[]> const &stripe) const noexcept {
    return to_span_of<T>(stripe_view(stripe).subspan(stripe_data_sz_));
  }

  auto stripe_allocate() const noexcept { return stripe_pool_->get(); }

  auto stripe_parity_allocate() const noexcept {
    return stripe_parity_pool_->get();
  }

  int stripe_write(uint64_t stripe_id_at, std::shared_ptr<write_query> wqd,
                   std::shared_ptr<write_query> wqp) noexcept;
  int stripe_write(uint64_t stripe_id_at,
                   std::shared_ptr<write_query> wq) noexcept;

  int read_data_skip_parity(uint64_t stripe_id_from,
                            std::shared_ptr<read_query> rq) noexcept;
  int read_stripe_data(uint64_t stripe_id_from,
                       std::shared_ptr<read_query> rq) noexcept;
  int read_stripe_parity(uint64_t stripe_id_from,
                         std::shared_ptr<read_query> rq) noexcept;

  ~Target() = default;

  virtual uint64_t
  stripe_id_to_parity_id(uint64_t stripe_id) const noexcept = 0;

private:
  int process(uint64_t stripe_id, std::shared_ptr<write_query> wq) noexcept;

public:
  explicit Target(uint64_t strip_sz,
                  std::vector<std::shared_ptr<IRWHandler>> hs);

  int process(std::shared_ptr<read_query> rq) noexcept;
  int process(std::shared_ptr<write_query> wq) noexcept;

protected:
  uint64_t strip_sz_;
  uint64_t stripe_data_sz_;
  std::vector<std::shared_ptr<IRWHandler>> hs_;
  bitset_locker<uint64_t, mm::allocator::cache_line_aligned_allocator<uint64_t>>
      stripe_w_locker_;

  mutable std::unique_ptr<mm::mem_chunk_pool> stripe_pool_;
  mutable std::unique_ptr<mm::mem_chunk_pool> stripe_parity_pool_;

  boost::dynamic_bitset<uint64_t> stripe_parity_coherency_state_;
  std::vector<std::pair<uint64_t, std::shared_ptr<write_query>>> wqs_pending_;
};

} // namespace ublk::raidsp
