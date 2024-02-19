#pragma once

#include <utility>

#include <linux/ublkdrv/cellc.h>
#include <linux/ublkdrv/cmdb.h>
#include <linux/ublkdrv/cmdb_ack.h>

#include "cfq_popper.hpp"
#include "cfq_pusher.hpp"

namespace ublk {

/* clang-format off */
using qublkcmd_t =
    cfq_popper<ublkdrv_cmd, decltype(std::declval<ublkdrv_cellc>().cmdb_head), decltype(std::declval<ublkdrv_cmdb>().tail)>;
using qublkcmd_ack_t =
    cfq_pusher<ublkdrv_cmd_ack, decltype(std::declval<ublkdrv_cellc>().cmdb_ack_head), decltype(std::declval<ublkdrv_cmdb_ack>().tail)>;
/* clang-format on */

} // namespace ublk
