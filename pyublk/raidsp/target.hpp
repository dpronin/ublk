#pragma once

#include <cstddef>
#include <cstdint>

#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include <boost/dynamic_bitset/dynamic_bitset.hpp>

#include "read_query.hpp"
#include "rw_handler_interface.hpp"
#include "sector.hpp"
#include "span.hpp"
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

  template <typename T = std::byte>
  auto cached_stripe_view(
      mm::uptrwd<std::byte[]> const &cached_stripe) const noexcept {
    return to_span_of<T>({cached_stripe.get(), stripe_data_sz_ + strip_sz_});
  }

  template <typename T = std::byte>
  auto cached_stripe_data_view(
      mm::uptrwd<std::byte[]> const &cached_stripe) const noexcept {
    return to_span_of<T>(
        cached_stripe_view(cached_stripe).subspan(0, stripe_data_sz_));
  }

  template <typename T = std::byte>
  auto cached_stripe_parity_view(
      mm::uptrwd<std::byte[]> const &cached_stripe) const noexcept {
    return to_span_of<T>(
        cached_stripe_view(cached_stripe).subspan(stripe_data_sz_));
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
  std::function<mm::uptrwd<std::byte[]>()> cached_stripe_generator_;
  std::function<mm::uptrwd<std::byte[]>()> cached_stripe_parity_generator_;

private:
  boost::dynamic_bitset<uint64_t> stripe_parity_coherency_state_;
  boost::dynamic_bitset<uint64_t> stripe_write_lock_state_;
  std::vector<std::pair<uint64_t, std::shared_ptr<write_query>>> wqs_pending_;
};

} // namespace ublk::raidsp
