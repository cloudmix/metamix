#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#include "application_context.h"
#include "clear_input.h"
#include "clock.h"
#include "input_manager.h"
#include "log.h"
#include "metadata.h"
#include "metadata_queue.h"
#include "proc/controller.h"
#include "proc/extractor.h"
#include "proc/injector.h"
#include "program_options.h"
#include "supervisor.h"
#include "user_defined_input.h"

namespace log = metamix::log;
using namespace metamix;
using namespace metamix::proc;

int
main(int argc, char *argv[])
{
  try {
    log::init();
    log::set_thread_name("main");

    std::shared_ptr<ProgramOptions> options(ProgramOptions::parse(argc, argv));
    options->validate();

    log::set_filter(options->logging_level, options->logging_thread);

    auto meta_queue = std::make_shared<ApplicationMetadataQueueGroup>();
    auto clock = std::make_shared<Clock>();

    std::vector<std::unique_ptr<AbstractInput>> inputs;

    inputs.push_back(std::make_unique<ClearInput>(0)); // must be first!

    for (auto &is : options->user_inputs) {
      is.id = inputs.size();
      inputs.push_back(std::make_unique<UserDefinedInput>(is));
    }

    auto input_manager = std::make_shared<InputManager>(std::move(inputs));

    if (options->start_input_name) {
      InputManager::InputsChangeset<std::string> changeset;
      InputCapabilities::Kinds::for_each([&](auto k) {
        using K = decltype(k);
        K::get(changeset) = *options->start_input_name;
      });
      input_manager->set_current_inputs(changeset);
    }

    auto ctx = std::make_shared<ApplicationContext>(
      // clang-format off
      std::move(meta_queue),
      std::move(clock),
      std::move(input_manager),
      std::move(options)
      // clang-format on
    );

    std::vector<std::thread> primary_threads, secondary_threads;
    primary_threads.reserve(ctx->input_manager->size());

    secondary_threads.emplace_back(supervised(controller, !ctx->options->norestart), ctx);

    for (const auto &is : ctx->options->user_inputs) {
      primary_threads.emplace_back(supervised(extractor, !ctx->options->norestart), is.name, ctx);
    }

    primary_threads.emplace_back(supervised(injector, !ctx->options->norestart), ctx);

    for (auto &th : primary_threads) {
      if (th.joinable()) {
        th.join();
      }
    }

    // Primary threads finished, let signal secondary threads that out time has end.
    LOG(debug) << "Exiting...";
    ctx->exit();

    for (auto &th : secondary_threads) {
      if (th.joinable()) {
        th.join();
      }
    }

    return EXIT_SUCCESS;
  } catch (const std::exception &ex) {
    LOG(fatal) << ex.what();
    return EXIT_FAILURE;
  }
}
