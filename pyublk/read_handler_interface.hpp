#pragma once

#include <memory>

#include "submitter_interface.hpp"
#include "read_query.hpp"

namespace ublk {
using IReadHandler = ISubmitter<int(std::shared_ptr<read_query>) noexcept>;
} // namespace ublk
