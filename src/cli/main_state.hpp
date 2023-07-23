#pragma once

#include <memory>

#include "bdev_mapper_interface.hpp"
#include "bdev_unmapper_interface.hpp"
#include "cmd_state.hpp"
#include "target_creator_interface.hpp"
#include "target_destroyer_interface.hpp"
#include "types.hpp"

namespace ublk::cli {

class MainState : public CmdState {
public:
  explicit MainState(std::shared_ptr<ITargetCreator> target_creator,
                     std::shared_ptr<ITargetDestroyer> target_destroyer,
                     std::shared_ptr<IBdevMapper> bdev_mapper,
                     std::shared_ptr<IBdevUnmapper> bdev_unmapper,
                     std::shared_ptr<bool> finish_token);
  ~MainState() override = default;

  MainState(MainState const &) = delete;
  MainState &operator=(MainState const &) = delete;

  MainState(MainState &&) = delete;
  MainState &operator=(MainState &&) = delete;

  [[nodiscard]] prompt_t prompt() const override;

protected:
  [[nodiscard]] std::string default_colored(std::string_view input) const;
};

} // namespace ublk::cli
