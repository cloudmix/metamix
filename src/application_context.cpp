#include "application_context.h"

#include "log.h"
#include "program_options.h"

namespace metamix {

ApplicationContext::ApplicationContext(std::shared_ptr<ApplicationMetadataQueueGroup> meta_queue,
                                       std::shared_ptr<Clock> clock,
                                       std::shared_ptr<InputManager> input_manager,
                                       std::shared_ptr<const ProgramOptions> options)
  : meta_queue(std::move(meta_queue))
  , clock(std::move(clock))
  , input_manager(std::move(input_manager))
  , options(options)
  , m_ts_adjustment(options->output.ts_adjustment)
{}

void
ApplicationContext::exit()
{
  bool was_running = m_running.exchange(false);
  if (was_running) {
    on_exit();
  }
}

void
ApplicationContext::ts_adjustment(ClockTS value)
{
  auto prev_val = m_ts_adjustment.exchange(value.val);
  if (prev_val != value.val) {
    LOG(info) << "Changed injection TS adjustment to " << value.val;
  }
}
}
