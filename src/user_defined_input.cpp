#include "user_defined_input.h"

#include "h264/stdseis.h"
#include "log.h"
#include "metadata_queue.h"

using metamix::h264::build_empty_metadata;

namespace metamix {

std::optional<std::vector<Metadata<SeiKind>>>
UserDefinedInput::run_query_sei(ClockTS since_ts, ClockTS until_ts, const ApplicationContext &ctx)
{
  std::vector<Metadata<SeiKind>> v;

  int popped = ctx.meta_queue->pop_all<SeiKind>(m_spec.id, since_ts, until_ts, std::back_inserter(v));

  if (popped > 0) {
    return std::move(v);
  } else {
    return std::nullopt;
  }
}

void
UserDefinedInput::schedule_restart()
{
  LOG(info) << "Input :" << m_spec.name << " restart scheduled.";
  is_restart_scheduled(true);
}
}
