#pragma once

#include <cstddef>

#include <memory>
#include <span>

#include <linux/ublk/cellc.h>
#include <linux/ublk/cmd.h>

#include "handler_interface.hpp"
#include "read_handler_interface.hpp"

namespace cfq {

class CmdReadHandler : public IHandler<int(ublk_cmd_read, ublk_cellc const &,
                                           std::span<std::byte>) noexcept> {
public:
  explicit CmdReadHandler(std::shared_ptr<IReadHandler> reader);
  ~CmdReadHandler() override = default;

  CmdReadHandler(CmdReadHandler const &) = default;
  CmdReadHandler &operator=(CmdReadHandler const &) = default;

  CmdReadHandler(CmdReadHandler &&) = default;
  CmdReadHandler &operator=(CmdReadHandler &&) = default;

  int handle(ublk_cmd_read cmd, ublk_cellc const &cellc,
             std::span<std::byte> cells) noexcept override;

private:
  std::shared_ptr<IReadHandler> reader_;
};

} // namespace cfq
