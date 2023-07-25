#pragma once

#include <type_traits>

#include <linux/ublk/cellc.h>
#include <linux/ublk/cmdb.h>
#include <linux/ublk/cmdb_ack.h>

#include "cfq_popper.hpp"
#include "cfq_pusher.hpp"

namespace ublk {

/* clang-format off */
using qublkcmd_t =
    cfq_popper<ublk_cmd, decltype(std::declval<ublk_cellc>().cmdb_head), decltype(std::declval<ublk_cmdb>().tail)>;
using qublkcmd_ack_t =
    cfq_pusher<ublk_cmd_ack, decltype(std::declval<ublk_cellc>().cmdb_ack_head), decltype(std::declval<ublk_cmdb_ack>().tail)>;
/* clang-format on */

} // namespace ublk
