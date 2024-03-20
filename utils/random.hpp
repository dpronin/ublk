#pragma once

#include <cstddef>

#include <random>

namespace ublk {

constexpr inline auto make_random_bytes_generator = [] {
  return [rd = std::random_device{}] mutable {
    return static_cast<std::byte>(rd());
  };
};

} // namespace ublk
