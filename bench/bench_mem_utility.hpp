#include <cstddef>

#include "mm/mem.hpp"
#include "mm/mem_types.hpp"

#include "utils/page.hpp"
#include "utils/utility.hpp"

namespace ublk::bench {

inline auto make_buffer(size_t sz) {
  auto const page_size = get_page_size();
  auto buf = make_unique_bytes(mm::alloc_mode_mmap{}, sz);
  /* initialize a first byte of each page to prevent page faults while
   * benchmarking */
  for (size_t i = 0, n = div_round_up(sz, page_size); i < n; ++i)
    buf[i * page_size] = static_cast<std::byte>(std::rand());
  return buf;
}

} // namespace ublk::bench
