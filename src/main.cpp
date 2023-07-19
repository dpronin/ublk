#include <cstdlib>

#include <exception>
#include <iostream>
#include <memory>
#include <string>

#include "cli_ctx.hpp"
#include "cmd_invoker.hpp"
#include "color.hpp"
#include "main_state.hpp"
#include "master.hpp"
#include "readline.hpp"

using namespace cfq;

int main(int argc, char const *argv[]) {
  auto master = std::make_shared<Master>();
  auto finish_token = std::make_shared<bool>(false);
  auto ctx = std::make_shared<CliCtx>(
      Readline::instance(), finish_token,
      std::make_unique<MainState>(master, finish_token));
  for (CmdInvoker invoker{ctx}; invoker;) {
    try {
      invoker(ctx->ureadcmd());
    } catch (std::exception const &ex) {
      std::cerr << cred(ex.what()) << '\n';
    } catch (...) {
      std::cerr << cred("unknown error occurred") << '\n';
      break;
    }
  }
  return EXIT_SUCCESS;
}
