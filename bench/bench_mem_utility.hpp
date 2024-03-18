#include <cstddef>
#include <cstdlib>

#include <algorithm>
#include <ranges>

#include "mm/mem.hpp"
#include "mm/mem_types.hpp"

#include "utils/page.hpp"
#ifndef __cpp_lib_ranges_stride
#include "utils/utility.hpp"
#endif

namespace ublk::bench {

inline auto make_buffer(size_t sz) {
  auto const page_size = get_page_size();
  auto buf = make_unique_bytes(mm::alloc_mode_mmap{}, sz);
  /* initialize a first byte of each page to prevent page faults while
   * benchmarking */
  auto const gen = [] { return static_cast<std::byte>(std::rand()); };
#ifdef __cpp_lib_ranges_stride
  std::ranges::generate(
      std::span{buf.get(), sz} | std::views::stride(page_size), gen);
#else
  for (size_t i = 0, n = div_round_up(sz, page_size); i < n; ++i)
    buf[i * page_size] = gen();
#endif
  return buf;
}

} // namespace ublk::bench
