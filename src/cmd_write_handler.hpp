#pragma once

#include <cstddef>

#include <memory>
#include <span>

#include <linux/ublk/cellc.h>
#include <linux/ublk/cmd.h>

#include "handler_interface.hpp"
#include "write_handler_interface.hpp"

namespace cfq {

class CmdWriteHandler : public IHandler<int(ublk_cmd_write) noexcept> {
public:
  explicit CmdWriteHandler(std::shared_ptr<ublk_cellc const> cellc,
                           std::span<std::byte const> cells,
                           std::shared_ptr<IWriteHandler> writer);
  ~CmdWriteHandler() override = default;

  CmdWriteHandler(CmdWriteHandler const &) = default;
  CmdWriteHandler &operator=(CmdWriteHandler const &) = default;

  CmdWriteHandler(CmdWriteHandler &&) = default;
  CmdWriteHandler &operator=(CmdWriteHandler &&) = default;

  int handle(ublk_cmd_write cmd) noexcept override;

private:
  std::shared_ptr<ublk_cellc const> cellc_;
  std::span<std::byte const> cells_;
  std::shared_ptr<IWriteHandler> writer_;
};

} // namespace cfq
