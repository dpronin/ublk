#pragma once

#include <cstddef>
#include <cstdint>

#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include <boost/dynamic_bitset/dynamic_bitset.hpp>

#include "mm/cache_line_aligned_allocator.hpp"
#include "mm/mem_chunk_pool.hpp"
#include "mm/mem_types.hpp"

#include "utils/bitset_locker.hpp"
#include "utils/span.hpp"
#include "utils/utility.hpp"

#include "read_query.hpp"
#include "rw_handler_interface.hpp"
#include "write_query.hpp"

#include "backend.hpp"

namespace ublk::raidsp {

class acceptor final {
public:
  explicit acceptor(
      uint64_t strip_sz, std::vector<std::shared_ptr<IRWHandler>> hs,
      std::function<uint64_t(uint64_t stripe_id)> stripe_id_to_parity_id);
  ~acceptor() = default;

  bool is_stripe_parity_coherent(uint64_t stripe_id) const noexcept;

  int process(std::shared_ptr<read_query> rq) noexcept;
  int process(std::shared_ptr<write_query> wq) noexcept;

private:
  int stripe_incoherent_parity_write(uint64_t stripe_id_at,
                                     std::shared_ptr<write_query> wqd,
                                     std::shared_ptr<write_query> wqp) noexcept;

  int stripe_coherent_parity_write(uint64_t stripe_id_at,
                                   std::shared_ptr<write_query> wqd,
                                   std::shared_ptr<write_query> wqp) noexcept;

  int stripe_write(uint64_t stripe_id_at, std::shared_ptr<write_query> wqd,
                   std::shared_ptr<write_query> wqp) noexcept;

  int stripe_data_write(uint64_t stripe_id_at,
                        std::shared_ptr<write_query> wq) noexcept;

  constexpr static inline auto kCachedStripeAlignment =
      backend::kAlignmentRequiredMin;
  static_assert(is_aligned_to(kCachedStripeAlignment,
                              alignof(std::max_align_t)));

  constexpr static inline auto kCachedParityAlignment =
      backend::kAlignmentRequiredMin;
  static_assert(is_aligned_to(kCachedParityAlignment,
                              alignof(std::max_align_t)));

  template <typename T = std::byte>
  auto stripe_view(mm::uptrwd<std::byte[]> const &stripe) const noexcept {
    return to_span_of<T>(stripe_pool_->chunk_view(stripe));
  }

  template <typename T = std::byte>
  auto stripe_data_view(mm::uptrwd<std::byte[]> const &stripe) const noexcept {
    return to_span_of<T>(
        stripe_view(stripe).subspan(0, be_->static_cfg().stripe_data_sz));
  }

  template <typename T = std::byte>
  auto
  stripe_parity_view(mm::uptrwd<std::byte[]> const &stripe) const noexcept {
    return to_span_of<T>(
        stripe_view(stripe).subspan(be_->static_cfg().stripe_data_sz));
  }

  int process(uint64_t stripe_id, std::shared_ptr<write_query> wq) noexcept;

  std::unique_ptr<backend> be_;
  bitset_locker<uint64_t, mm::allocator::cache_line_aligned_allocator<uint64_t>>
      stripe_w_locker_;
  std::unique_ptr<mm::mem_chunk_pool> stripe_pool_;
  std::unique_ptr<mm::mem_chunk_pool> stripe_parity_pool_;

  boost::dynamic_bitset<uint64_t> stripe_parity_coherency_state_;
  std::vector<std::pair<uint64_t, std::shared_ptr<write_query>>> wqs_pending_;
};

} // namespace ublk::raidsp
