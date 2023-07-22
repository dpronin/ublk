#include <cstdlib>

#include <exception>
#include <iostream>
#include <memory>
#include <string>

#include "cli/cli_ctx.hpp"
#include "cli/cmd_invoker.hpp"
#include "cli/color.hpp"
#include "cli/main_state.hpp"
#include "cli/readline.hpp"

#include "bdev_creator.hpp"
#include "bdev_destroyer.hpp"
#include "master.hpp"

using namespace cfq;

int main(int argc, char const *argv[]) {
  auto master = std::make_shared<Master>();
  auto finish_token = std::make_shared<bool>(false);
  auto ctx = std::make_shared<cli::CliCtx>(
      cli::Readline::instance(),
      std::make_unique<cli::MainState>(std::make_unique<BdevCreator>(master),
                                       std::make_unique<BdevDestroyer>(master),
                                       finish_token));
  for (cli::CmdInvoker invoker{ctx}; !*finish_token;) {
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
