#pragma once

#include <cstdint>

#include <functional>
#include <memory>
#include <vector>

#include "mm/mem_types.hpp"

#include "utils/utility.hpp"

#include "rw_handler_interface.hpp"
#include "sector.hpp"

namespace ublk::raidsp {

class backend {
public:
  constexpr static inline auto kAlignmentRequiredMin = kSectorSz;
  static_assert(is_aligned_to(kAlignmentRequiredMin, kSectorSz));

  struct cfg_t {
    uint64_t strip_sz;
    uint64_t stripe_data_sz;
    uint64_t stripe_sz;
  };

  explicit backend(uint64_t strip_sz,
                   std::vector<std::shared_ptr<IRWHandler>> hs,
                   std::function<uint64_t(uint64_t stripe_id)> const
                       &stripe_id_to_parity_id);
  ~backend() = default;

  backend(backend const &) = delete;
  backend &operator=(backend const &) = delete;

  backend(backend &&) = default;
  backend &operator=(backend &&) = default;

  int data_skip_parity_read(uint64_t stripe_id_from,
                            std::shared_ptr<read_query> rq) noexcept;

  int stripe_data_read(uint64_t stripe_id_from,
                       std::shared_ptr<read_query> rq) noexcept;

  int stripe_parity_read(uint64_t stripe_id_from,
                         std::shared_ptr<read_query> rq) noexcept;

  int stripe_write(uint64_t stripe_id_at, std::shared_ptr<write_query> wqd,
                   std::shared_ptr<write_query> wqp) noexcept;

  cfg_t const &static_cfg() const noexcept { return *static_cfg_; }

private:
  std::vector<std::shared_ptr<IRWHandler>>
  handlers_generate(uint64_t stripe_id);

  uint64_t stripe_id_to_parity_id(uint64_t stripe_id) const noexcept {
    return stripe_id_to_parity_id_(stripe_id);
  }

  std::vector<std::shared_ptr<IRWHandler>> hs_;
  std::function<uint64_t(uint64_t stripe_id)> stripe_id_to_parity_id_;

  mm::uptrwd<cfg_t const> static_cfg_;
};

} // namespace ublk::raidsp
