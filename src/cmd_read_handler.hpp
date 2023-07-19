#pragma once

#include <cstddef>

#include <memory>
#include <span>

#include <linux/ublk/cellc.h>
#include <linux/ublk/cmd.h>

#include "cmd_handler_interface.hpp"
#include "read_handler_interface.hpp"

namespace cfq {

class CmdReadHandler : public ICmdHandler<const ublk_cmd_read> {
public:
  explicit CmdReadHandler(std::shared_ptr<ublk_cellc const> cellc,
                          std::span<std::byte> cells,
                          std::shared_ptr<IReadHandler> reader);
  ~CmdReadHandler() override = default;

  CmdReadHandler(CmdReadHandler const &) = default;
  CmdReadHandler &operator=(CmdReadHandler const &) = default;

  CmdReadHandler(CmdReadHandler &&) = default;
  CmdReadHandler &operator=(CmdReadHandler &&) = default;

  int handle(ublk_cmd_read const &cmd) noexcept override;

private:
  std::shared_ptr<ublk_cellc const> cellc_;
  std::span<std::byte> cells_;
  std::shared_ptr<IReadHandler> reader_;
};

} // namespace cfq
