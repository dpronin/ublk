#include "main_state.hpp"

#include <memory>
#include <utility>

#include "color.hpp"
#include "types.hpp"

#include "cmd_bdev_create.hpp"
#include "cmd_bdev_destroy.hpp"
#include "cmd_quit.hpp"

using namespace cfq;

// clang-format off
MainState::MainState(std::shared_ptr<Master> master, std::shared_ptr<bool> finish_token)
    : CmdState({
        { "create",  { "[bdev_suffix] [target]" }, "Creates a new block device mapped to a target", [=] (args_t args)                  { return std::make_unique<CmdBdevCreate>(std::move(args), master); } },
        { "destroy", { "[bdev_suffix]"          }, "Destroys an exising block device",              [=] (args_t args)                  { return std::make_unique<CmdBdevDestroy>(std::move(args), master); } },
        { "quit",    {                          }, "Quits the application",                         [=] (args_t args [[maybe_unused]]) { return std::make_unique<CmdQuit>(finish_token); }}
    })
// clang-format on
{
  assert(master);
  assert(finish_token);
}

prompt_t MainState::prompt() const { return default_colored("cfq > "); }

std::string MainState::default_colored(std::string_view input) const {
  return cbrown(input);
}
