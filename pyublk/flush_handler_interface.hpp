#pragma once

#include <memory>

#include "submitter_interface.hpp"
#include "flush_query.hpp"

namespace ublk {
using IFlushHandler =
    ISubmitter<int(std::shared_ptr<flush_query>) noexcept>;
} // namespace ublk
