#pragma once

#include <cstddef>

#include <memory>
#include <span>

#include <linux/ublkdrv/celld.h>
#include <linux/ublkdrv/cmd.h>

#include "handler_interface.hpp"
#include "write_handler_interface.hpp"

namespace ublk {

class CmdWriteHandler
    : public IHandler<int(ublkdrv_cmd_write, std::span<ublkdrv_celld const>,
                          std::span<std::byte const>) noexcept> {
public:
  explicit CmdWriteHandler(std::shared_ptr<IWriteHandler> writer);
  ~CmdWriteHandler() override = default;

  CmdWriteHandler(CmdWriteHandler const &) = default;
  CmdWriteHandler &operator=(CmdWriteHandler const &) = default;

  CmdWriteHandler(CmdWriteHandler &&) = default;
  CmdWriteHandler &operator=(CmdWriteHandler &&) = default;

  int handle(ublkdrv_cmd_write cmd, std::span<ublkdrv_celld const> cellds,
             std::span<std::byte const> cells) noexcept override;

private:
  std::shared_ptr<IWriteHandler> writer_;
};

} // namespace ublk
