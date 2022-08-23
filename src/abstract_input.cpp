#include "abstract_input.h"

#include "h264/stdseis.h"

using metamix::h264::build_empty_metadata;

namespace metamix {

std::optional<std::vector<Metadata<SeiKind>>>
AbstractInput::run_query_sei(ClockTS, ClockTS, const ApplicationContext &)
{
  return std::nullopt;
}

std::optional<std::vector<Metadata<ScteKind>>>
AbstractInput::run_query_scte(ClockTS, ClockTS, const ApplicationContext &)
{
  return std::nullopt;
}

std::vector<Metadata<SeiKind>>
AbstractInput::build_empty_sei(ClockTS since_ts, ClockTS until_ts)
{
  return std::vector{ metamix::h264::build_empty_metadata(spec().id, std::max(since_ts, until_ts - 1_clock)) };
}

std::vector<Metadata<ScteKind>> AbstractInput::build_empty_scte([[maybe_unused]] ClockTS since_ts,
                                                                [[maybe_unused]] ClockTS until_ts)
{
  throw std::runtime_error("not implemented yet");
}
}
