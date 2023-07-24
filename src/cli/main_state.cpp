#include "main_state.hpp"

#include <memory>
#include <utility>

#include "color.hpp"
#include "types.hpp"

#include "cmds/bdev_map.hpp"
#include "cmds/bdev_unmap.hpp"
#include "cmds/quit.hpp"
#include "cmds/target_create.hpp"
#include "cmds/target_destroy.hpp"

using namespace ublk::cli;

/* clang-format off */
MainState::MainState(std::shared_ptr<ITargetCreator> target_creator,
                     std::shared_ptr<ITargetDestroyer> target_destroyer,
                     std::shared_ptr<IBdevMapper> bdev_mapper,
                     std::shared_ptr<IBdevUnmapper> bdev_unmapper,
                     std::shared_ptr<bool> finish_token)
    : ShellState({
        { "target-create",  { "name", "path", "capacity-sectors"          }, "Creates a new target specified by the type",     [target_creator]   (args_t args)                  { return std::make_unique<cmds::TargetCreate>(std::move(args), target_creator); } },
        { "target-destroy", { "name"                                      }, "Destroys an exising block device",               [target_destroyer] (args_t args)                  { return std::make_unique<cmds::TargetDestroy>(std::move(args), target_destroyer); } },
        { "bdev-map",       { "bdev-suffix", "target-name", "[read_only]" }, "Maps a new block device to the existing target", [bdev_mapper]      (args_t args)                  { return std::make_unique<cmds::BdevMap>(std::move(args), bdev_mapper); } },
        { "bdev-unmap",     { "bdev-suffix"                               }, "Unmaps an exising block device from the target", [bdev_unmapper]    (args_t args)                  { return std::make_unique<cmds::BdevUnmap>(std::move(args), bdev_unmapper); } },
        { "quit",           {                                             }, "Quits the application",                          [finish_token]     (args_t args [[maybe_unused]]) { return std::make_unique<cmds::Quit>(finish_token); }}
    })
/* clang-format on */
{
  assert(target_creator);
  assert(target_destroyer);
  assert(bdev_mapper);
  assert(bdev_unmapper);
  assert(finish_token);
}

prompt_t MainState::prompt() const { return default_colored("ublk > "); }

std::string MainState::default_colored(std::string_view input) const {
  return cbrown(input);
}
