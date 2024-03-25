#pragma once

#include "concepts.hpp"

namespace ublk {

constexpr auto make_less_than{
    [](is_trivially_copyable auto const &bound) {
      return [=](auto const &x) { return x < bound; };
    },
};

} // namespace ublk
