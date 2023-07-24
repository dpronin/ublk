#include <cstdlib>

#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <system_error>

#include "cli/cmds/invoker.hpp"

#include "cli/cli_ctx.hpp"
#include "cli/color.hpp"
#include "cli/main_state.hpp"
#include "cli/readline.hpp"

#include "bdev_mapper.hpp"
#include "bdev_unmapper.hpp"
#include "master.hpp"
#include "target_creator.hpp"
#include "target_destroyer.hpp"

using namespace ublk;

int main([[maybe_unused]] int argc, [[maybe_unused]] char const *argv[]) {
  auto master = std::make_shared<Master>();
  auto finish_token = std::make_shared<bool>(false);
  auto ctx = std::make_shared<cli::CliCtx>(
      cli::Readline::instance(),
      std::make_unique<cli::MainState>(
          std::make_unique<TargetCreator>(master),
          std::make_unique<TargetDestroyer>(master),
          std::make_unique<BdevMapper>(master),
          std::make_unique<BdevUnmapper>(master), finish_token));
  for (auto invoker = cli::cmds::Invoker{}; !*finish_token;) {
    try {
      invoker(ctx->ureadcmd());
    } catch (std::exception const &ex) {
      std::cerr << cli::cred(ex.what()) << '\n';
    } catch (...) {
      std::cerr << cli::cred("unknown error occurred") << '\n';
      break;
    }
  }
  return EXIT_SUCCESS;
}
