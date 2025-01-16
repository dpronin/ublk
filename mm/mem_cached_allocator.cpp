#include "mem_cached_allocator.hpp"

#include <cstdint>

#include <algorithm>
#include <utility>

#include <gsl/assert>

#include "utils/utility.hpp"

#include "mem.hpp"

namespace ublk::mm {

void *mem_cached_allocator::allocate_aligned(size_t alignment,
                                             size_t sz) noexcept {
  Expects(is_power_of_2(alignment));
  Expects(sz);

  alignment = std::max(alignment, alignof(std::max_align_t));
  auto ftbl_it = ftbl_.lower_bound(alignment);
  if (ftbl_it == ftbl_.end())
    ftbl_it = ftbl_.emplace_hint(ftbl_.end(), alignment, free_desc_table_t{});
  alignment = ftbl_it->first;

  sz = std::max(sz, alignment);
  auto &ftbl_desc = ftbl_it->second;
  auto ftbl_desc_it = ftbl_desc.lower_bound(sz);
  if (ftbl_desc_it == ftbl_desc.end())
    ftbl_desc_it = ftbl_desc.emplace_hint(ftbl_desc.end(), sz, free_desc{});
  sz = ftbl_desc_it->first;

  auto &free_chunks = ftbl_desc_it->second.free_chunks;

  if (free_chunks.empty())
    free_chunks.push(get_unique_bytes_generator(alignment, sz)());

  auto uptr{std::move(free_chunks.top())};
  free_chunks.pop();

  auto *p = uptr.get();
  [[maybe_unused]] auto const [it, emplaced] = btbl_.emplace(
      reinterpret_cast<uintptr_t>(p), busy_desc{std::move(uptr), ftbl_desc_it});
  Ensures(emplaced);

  return p;
}

void mem_cached_allocator::free(void *p [[maybe_unused]]) noexcept {
  auto const it = btbl_.find(reinterpret_cast<uintptr_t>(p));
  Expects(it != btbl_.end());
  it->second.ftbl_desc_it->second.free_chunks.push(std::move(it->second.chunk));
  btbl_.erase(it);
}

} // namespace ublk::mm
