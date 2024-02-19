#pragma once

#include <cstddef>

#include <memory>
#include <span>

#include <linux/ublkdrv/celld.h>
#include <linux/ublkdrv/cmd.h>

#include "handler_interface.hpp"
#include "read_handler_interface.hpp"

namespace ublk {

class CmdReadHandler
    : public IHandler<int(ublkdrv_cmd_read, std::span<ublkdrv_celld const>,
                          std::span<std::byte>) noexcept> {
public:
  explicit CmdReadHandler(std::shared_ptr<IReadHandler> reader);
  ~CmdReadHandler() override = default;

  CmdReadHandler(CmdReadHandler const &) = default;
  CmdReadHandler &operator=(CmdReadHandler const &) = default;

  CmdReadHandler(CmdReadHandler &&) = default;
  CmdReadHandler &operator=(CmdReadHandler &&) = default;

  int handle(ublkdrv_cmd_read cmd, std::span<ublkdrv_celld const> cellds,
             std::span<std::byte> cells) noexcept override;

private:
  std::shared_ptr<IReadHandler> reader_;
};

} // namespace ublk
