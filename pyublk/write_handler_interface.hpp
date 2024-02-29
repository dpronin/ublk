#pragma once

#include <memory>

#include "submitter_interface.hpp"
#include "write_query.hpp"

namespace ublk {
using IWriteHandler =
    ISubmitter<int(std::shared_ptr<write_query>) noexcept>;
} // namespace ublk
