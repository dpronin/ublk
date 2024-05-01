#pragma once

#include <memory>

#include "submitter_interface.hpp"
#include "discard_query.hpp"

namespace ublk {
using IDiscardHandler =
    ISubmitter<int(std::shared_ptr<discard_query>) noexcept>;
} // namespace ublk
