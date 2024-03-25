#pragma once

#include <cassert>
#include <cstddef>
#include <cstring>

#include <fcntl.h>
#include <sys/mman.h>

#include <algorithm>
#include <functional>
#include <new>
#include <type_traits>
#include <utility>

#include "utils/algo.hpp"
#include "utils/concepts.hpp"
#include "utils/page.hpp"
#include "utils/random.hpp"
#include "utils/size_units.hpp"
#include "utils/utility.hpp"

#include "mem_types.hpp"

namespace ublk::mm {

template <typename Target, typename Source = void>
mem_t<Target> convert(mem_t<Source> from) {
  static_assert((std::is_trivial_v<Source> || std::is_void_v<Source>) &&
                    std::is_trivial_v<Target>,
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

template <typename Target = void>
mem_t<Target> mmap(size_t sz, int prot = PROT_READ | PROT_WRITE, int fd = -1,
                   int flags = 0, long offset = 0) noexcept {
  static_assert(std::is_trivial_v<Target>, "Target must be a trivial type");

  void *p = ::mmap(nullptr, sz, prot, flags | (-1 == fd ? MAP_ANONYMOUS : 0),
                   fd, offset);
  if (p == MAP_FAILED) {
    return {};
  }

  try {
    /*
     * Protect the mapping by smart pointer to gracefully unmap the region when
     * last reference is lost
     */
    using T = std::remove_extent_t<Target>;
    return {
        reinterpret_cast<T *>(p),
        [sz](T *p) {
          munmap(
              const_cast<void *>(reinterpret_cast<std::conditional_t<
                                     std::is_const_v<std::remove_pointer_t<T>>,
                                     void const, void> *>(p)),
              sz);
        },
    };
  } catch (...) {
  }

  return {};
}

namespace detail {

inline uptrwd<std::byte[]> make_unique_aligned_bytes(alloc_mode_new,
                                                     size_t align, size_t sz) {
  assert(is_power_of_2(align));
  return {
      new (std::align_val_t{align}) std::byte[sz]{},
      [align](std::byte p[]) { operator delete[](p, std::align_val_t{align}); },
  };
}

} // namespace detail

template <typename Target = void>
mem_t<Target> mmap_private(size_t sz, int prot = PROT_READ | PROT_WRITE,
                           int fd = -1, int flags = 0,
                           long offset = 0) noexcept {
  return mmap<Target>(sz, prot, fd, MAP_PRIVATE | flags, offset);
}

template <typename Target = void>
mem_t<Target> mmap_shared(size_t sz, int prot = PROT_READ | PROT_WRITE,
                          int fd = -1, int flags = 0,
                          long offset = 0) noexcept {
  return mmap<Target>(sz, prot, fd, MAP_SHARED | flags, offset);
}

template <typename T, typename... Args>
uptrwd<T> make_unique_aligned(size_t align, Args &&...args) {
  return {
      new (std::align_val_t{align}) T{std::forward<Args>(args)...},
      [align](auto p) {
        std::destroy_at(p);
        operator delete(p, std::align_val_t{align});
      },
  };
}

inline std::unique_ptr<std::byte[]> make_unique_zeroed_bytes(size_t sz) {
  return std::make_unique<std::byte[]>(sz);
}

template <typename T>
inline std::unique_ptr<T[]> make_unique_for_overwrite_array(size_t sz) {
  return std::make_unique_for_overwrite<T[]>(sz);
}

inline std::unique_ptr<std::byte[]> make_unique_for_overwrite_bytes(size_t sz) {
  return make_unique_for_overwrite_array<std::byte>(sz);
}

inline std::unique_ptr<std::byte[]> make_unique_randomized_bytes(size_t sz) {
  auto buf{std::make_unique_for_overwrite<std::byte[]>(sz)};
  auto gen = make_random_bytes_generator();
  std::generate_n(buf.get(), sz, std::ref(gen));
  return buf;
}

template <typename T>
inline std::unique_ptr<T[]> make_unique_array(alloc_mode_new, size_t sz) {
  return std::make_unique<T[]>(sz);
}

inline std::unique_ptr<std::byte[]> make_unique_bytes(alloc_mode_new,
                                                      size_t sz) {
  return make_unique_array<std::byte>(alloc_mode_new{}, sz);
}

template <is_trivially_copyable T>
inline auto duplicate_unique(std::unique_ptr<T[]> const &p, size_t sz) {
  auto p_dup{mm::make_unique_for_overwrite_array<T>(sz)};
  algo::copy(std::span<std::byte const>{p.get(), sz},
             std::span<std::byte>{p_dup.get(), sz});
  return p_dup;
}

inline uptrwd<std::byte[]> make_unique_bytes(alloc_mode_mmap param, size_t sz) {
  return mmap_private<std::byte[]>(sz, PROT_READ | PROT_WRITE, -1, param.flags,
                                   0);
}

inline auto get_unique_bytes_generator(size_t alignment, size_t chunk_sz) {
  std::function<uptrwd<std::byte[]>()> gen;
  if (chunk_sz < get_page_size()) {
    gen = [=] {
      return detail::make_unique_aligned_bytes(
          alloc_mode_new{}, std::max(alignment, alignof(std::max_align_t)),
          chunk_sz);
    };
  } else {
    alloc_mode_mmap mode{.flags = 0};
    auto generic_gen = [=] { return make_unique_bytes(mode, chunk_sz); };
    if (!(chunk_sz < 2_MiB)) {
      mode.flags = MAP_HUGETLB | (21 << MAP_HUGE_SHIFT);
      gen = [=] {
        if (auto huge = make_unique_bytes(mode, chunk_sz))
          return huge;
        return generic_gen();
      };
    } else {
      gen = generic_gen;
    }
  }
  return gen;
}

} // namespace ublk::mm
