#include "main_state.hpp"

#include <memory>
#include <utility>

#include "color.hpp"
#include "types.hpp"

#include "cmd_bdev_create.hpp"
#include "cmd_bdev_destroy.hpp"
#include "cmd_quit.hpp"

using namespace cfq::cli;

// clang-format off
MainState::MainState(std::shared_ptr<IBdevCreator> bdev_creator,
                     std::shared_ptr<IBdevDestroyer> bdev_destroyer,
                     std::shared_ptr<bool> finish_token)
    : CmdState({
        { "create",  { "bdev_suffix capacity_sectors target [read_only]" }, "Creates a new block device mapped to a target", [bdev_creator]   (args_t args)                  { return std::make_unique<CmdBdevCreate>(std::move(args), bdev_creator); } },
        { "destroy", { "bdev_suffix"                                     }, "Destroys an exising block device",              [bdev_destroyer] (args_t args)                  { return std::make_unique<CmdBdevDestroy>(std::move(args), bdev_destroyer); } },
        { "quit",    {                                                   }, "Quits the application",                         [finish_token]   (args_t args [[maybe_unused]]) { return std::make_unique<CmdQuit>(finish_token); }}
    })
// clang-format on
{
  assert(bdev_creator);
  assert(bdev_destroyer);
  assert(finish_token);
}

prompt_t MainState::prompt() const { return default_colored("ublk > "); }

std::string MainState::default_colored(std::string_view input) const {
  return cbrown(input);
}
