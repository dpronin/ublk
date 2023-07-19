#pragma once

#include <memory>

#include "cmd_state.hpp"
#include "master.hpp"
#include "types.hpp"

namespace cfq {

class MainState : public CmdState {
public:
  explicit MainState(std::shared_ptr<Master> master,
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

} // namespace cfq
