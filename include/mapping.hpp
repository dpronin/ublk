#pragma once

#include <cstddef>
#include <cstring>

#include <fcntl.h>
#include <sys/mman.h>

#include <filesystem>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

#include <spdlog/spdlog.h>

#include "file.hpp"
#include "mem.hpp"

namespace cfq {

template <typename Target = void>
mem_t<Target> map_shared(size_t sz, int prot = PROT_READ | PROT_WRITE,
                         int fd = -1, long offset = 0) noexcept {
  static_assert(std::is_trivial_v<Target>, "Target must be a trivial type");

  void *p = mmap(nullptr, sz, prot, MAP_SHARED | (-1 == fd ? MAP_ANONYMOUS : 0),
                 fd, offset);
  if (p == MAP_FAILED) {
    spdlog::error("mmap() failed, reason: {}", strerror(errno));
    return {};
  }

  try {
    /*
     * Protect the mapping by smart pointer to gracefully unmap the region when
     * last reference is lost
     */
    return {
        static_cast<Target *>(p),
        [sz](Target *p) {
          munmap(const_cast<std::remove_const_t<Target> *>(p), sz);
        },
    };
  } catch (std::exception const &ex) {
    spdlog::error("mmap() failed, reason: {}", ex.what());
  } catch (...) {
    spdlog::error("mmap() failed, reason: unknown exception");
  }

  return {};
}

template <typename Target = void, typename... Args>
auto map_shared(size_t sz, int prot, long offset,
                std::filesystem::path const &path, int oflag = O_RDWR,
                Args... args) {
  return map_shared<Target>(sz, prot, *open(path, oflag, args...), offset);
}

} // namespace cfq
