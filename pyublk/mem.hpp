#pragma once

#include <cassert>
#include <cstddef>
#include <cstring>

#include <fcntl.h>
#include <sys/mman.h>

#include <filesystem>
#include <new>
#include <type_traits>
#include <utility>

#include "file.hpp"
#include "mem_types.hpp"
#include "page.hpp"
#include "utility.hpp"

namespace ublk {

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
                           int fd = -1, long offset = 0) noexcept {
  return mmap<Target>(sz, prot, fd, MAP_PRIVATE, offset);
}

template <typename Target = void>
mem_t<Target> mmap_shared(size_t sz, int prot = PROT_READ | PROT_WRITE,
                          int fd = -1, long offset = 0) noexcept {
  return mmap<Target>(sz, prot, fd, MAP_SHARED, offset);
}

template <typename Target = void, typename... Args>
mem_t<Target> mmap_shared(size_t sz, int prot, long offset,
                          std::filesystem::path const &path, int oflag = O_RDWR,
                          Args... args) {
  return mmap_shared<Target>(sz, prot, *open(path.c_str(), oflag, args...),
                             offset);
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

inline uptrwd<std::byte[]> make_unique_bytes(alloc_mode_new, size_t sz) {
  return std::make_unique<std::byte[]>(sz);
}

inline uptrwd<std::byte[]> make_unique_bytes(alloc_mode_mmap, size_t sz) {
  return mmap_private<std::byte[]>(sz);
}

inline auto get_unique_bytes_generator(size_t chunk_sz) {
  std::function<uptrwd<std::byte[]>()> gen;
  if (chunk_sz < get_page_size()) {
    gen = [=] {
      return detail::make_unique_aligned_bytes(
          alloc_mode_new{}, alignof(std::max_align_t), chunk_sz);
    };
  } else {
    gen = [=] { return make_unique_bytes(alloc_mode_mmap{}, chunk_sz); };
  }
  return gen;
}

} // namespace ublk