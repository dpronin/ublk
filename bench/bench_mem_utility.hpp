#include <cassert>
#include <cstddef>

#include <fcntl.h>
#include <new>
#include <sys/mman.h>
#include <unistd.h>

#include <bit>
#include <exception>
#include <functional>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

namespace ublk::bench {

struct alloc_mode_new {};
struct alloc_mode_mmap {};

template <typename T>
using memd_t = std::function<void(
    std::conditional_t<std::is_unbounded_array_v<T>, T, T *> p)>;
template <typename T> using mem_t = std::unique_ptr<T, memd_t<T>>;

namespace detail {

constexpr auto div_round_up(std::unsigned_integral auto x,
                            std::unsigned_integral auto y) noexcept {
  assert(0 != y);
  return (x + y - 1) / y;
}

inline size_t get_page_size() noexcept {
  static auto const page_size = sysconf(_SC_PAGESIZE);
  if (page_size < 0)
    std::terminate();
  return static_cast<size_t>(page_size);
}

inline mem_t<std::byte[]> make_unique_aligned_bytes(alloc_mode_new,
                                                    size_t align, size_t sz) {
  assert(std::has_single_bit(align));
  return {
      new (std::align_val_t{align}) std::byte[sz]{},
      [align](std::byte p[]) { operator delete[](p, std::align_val_t{align}); },
  };
}

template <typename Target, typename Source = void>
mem_t<Target> convert(mem_t<Source> from) {
  static_assert((std::is_trivial_v<Source> ||
                 std::is_void_v<Source>)&&std::is_trivial_v<Target>,
                "Source and Target must be trivial types");
  using T = std::remove_extent_t<Target>;
  auto *to = reinterpret_cast<T *>(from.release());
  return {
      to,
      [d = std::move(from.get_deleter())](T *p) {
        using S = std::remove_extent_t<Source>;
        d(reinterpret_cast<S *>(p));
      },
  };
}

} // namespace detail

inline mem_t<void> mmap(size_t sz, int prot = PROT_READ | PROT_WRITE,
                        int flags = 0, long offset = 0) {
  void *p = ::mmap(nullptr, sz, prot, flags | MAP_PRIVATE | MAP_ANONYMOUS, -1,
                   offset);
  if (p == MAP_FAILED)
    throw std::bad_alloc{};
  return {p, [sz](void *p) { munmap(const_cast<void *>(p), sz); }};
}

inline mem_t<std::byte[]> make_unique_bytes(alloc_mode_new, size_t sz) {
  return std::make_unique<std::byte[]>(sz);
}

inline mem_t<std::byte[]> make_unique_bytes(alloc_mode_mmap, size_t sz) {
  return detail::convert<std::byte[]>(mmap(sz));
}

inline auto make_buffer(size_t sz) {
  auto const page_size = detail::get_page_size();
  auto buf = make_unique_bytes(alloc_mode_mmap{}, sz);
  /* initialize a first byte of each page to prevent page faults while
   * benchmarking */
  for (size_t i = 0, n = detail::div_round_up(sz, page_size); i < n; ++i)
    buf[i * page_size] = static_cast<std::byte>(std::rand());
  return buf;
}

} // namespace ublk::bench
