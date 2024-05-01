#pragma once

#include <memory>

#include "read_query.hpp"
#include "submitter_interface.hpp"

namespace ublk {
using IRDQSubmitter = ISubmitter<int(std::shared_ptr<read_query>) noexcept>;
} // namespace ublk
