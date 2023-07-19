#pragma once

#include <memory>

#include "bdev_creator_interface.hpp"
#include "bdev_destroyer_interface.hpp"
#include "cmd_state.hpp"
#include "types.hpp"

namespace cfq::cli {

class MainState : public CmdState {
public:
  explicit MainState(std::shared_ptr<IBdevCreator> bdev_creator,
                     std::shared_ptr<IBdevDestroyer> bdev_destroyer,
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

} // namespace cfq::cli
