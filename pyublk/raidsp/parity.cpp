#include "parity.hpp"

#include <cstdint>

#include "algo.hpp"
#include "math.hpp"
#include "span.hpp"
#include "utility.hpp"

namespace ublk {

void parity_to(std::span<std::byte const> data, size_t parity_start_offset,
               std::span<std::byte> parity) noexcept {
  assert(is_multiple_of(data.size(), sizeof(uint64_t)));
  assert(is_multiple_of(parity_start_offset, sizeof(uint64_t)));
  assert(is_multiple_of(parity.size(), sizeof(uint64_t)));

  parity_start_offset = parity_start_offset % parity.size();

  auto data_u64 = to_span_of<uint64_t const>(data);
  auto const parity_u64 = to_span_of<uint64_t>(parity);

  if (auto const parity_u64_offset = parity_start_offset / sizeof(uint64_t)) {
    auto const chunk_u64_len =
        std::min(data_u64.size(), parity_u64.size() - parity_u64_offset);
    auto const parity_u64_chunk =
        parity_u64.subspan(parity_u64_offset, chunk_u64_len);
    auto const data_u64_chunk = data_u64.subspan(0, chunk_u64_len);
    math::xor_to(data_u64_chunk, parity_u64_chunk);
    data_u64 = data_u64.subspan(data_u64_chunk.size());
  }

  for (; !(data_u64.size() < parity_u64.size());
       data_u64 = data_u64.subspan(parity_u64.size())) {
    math::xor_to(data_u64.subspan(0, parity_u64.size()), parity_u64);
  }

  if (!data_u64.empty())
    math::xor_to(data_u64, parity_u64.subspan(0, data_u64.size()));
}

void parity_renew(std::span<std::byte const> data,
                  std::span<std::byte> parity) noexcept {
  algo::fill(to_span_of<uint64_t>(parity), UINT64_C(0));
  parity_to(data, parity);
}

} // namespace ublk
