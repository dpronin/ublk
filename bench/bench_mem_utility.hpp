#include <cstddef>
#include <cstdlib>

#include <algorithm>
#include <functional>
#include <ranges>

#include "mm/mem.hpp"
#include "mm/mem_types.hpp"

#include "sys/page.hpp"

#include "utils/random.hpp"
#ifndef __cpp_lib_ranges_stride
#include "utils/utility.hpp"
#endif

namespace ublk::bench {

inline auto make_buffer(size_t sz) {
  auto buf = make_unique_bytes(mm::alloc_mode_mmap{}, sz);

  auto const page_size = get_page_size();
  /* initialize a first byte of each page to prevent page faults while
   * benchmarking */
  auto gen = make_random_bytes_generator();
#ifdef __cpp_lib_ranges_stride
  std::ranges::generate(
      std::span{buf.get(), sz} | std::views::stride(page_size), std::ref(gen));
#else
  for (auto i : std::views::iota(0uz, div_round_up(sz, page_size)))
    buf[i * page_size] = gen();
#endif

  return buf;
}

} // namespace ublk::bench
