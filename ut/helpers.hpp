#pragma once

#include <cstddef>

#include <algorithm>
#include <memory>
#include <random>

namespace ublk::ut {

inline std::unique_ptr<std::byte[]> make_unique_random_bytes(size_t sz) {
  auto storage{std::make_unique_for_overwrite<std::byte[]>(sz)};
  std::generate_n(storage.get(), sz, [rd = std::random_device{}] mutable {
    return static_cast<std::byte>(rd());
  });
  return storage;
}

} // namespace ublk::ut
