#pragma once

#include <memory>

#include "flush_query.hpp"
#include "submitter_interface.hpp"

namespace ublk {
using IFLQSubmitter = ISubmitter<int(std::shared_ptr<flush_query>) noexcept>;
} // namespace ublk
