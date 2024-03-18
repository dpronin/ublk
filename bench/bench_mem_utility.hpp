#include <cstddef>

#include <ranges>

#include "mm/mem.hpp"
#include "mm/mem_types.hpp"

#include "utils/page.hpp"

namespace ublk::bench {

inline auto make_buffer(size_t sz) {
  auto const page_size = get_page_size();
  auto buf = make_unique_bytes(mm::alloc_mode_mmap{}, sz);
  auto const buf_span{std::span{buf.get(), sz}};
  /* initialize a first byte of each page to prevent page faults while
   * benchmarking */
  for (auto &b : buf_span | std::views::stride(page_size))
    b = static_cast<std::byte>(std::rand());
  return buf;
}

} // namespace ublk::bench
