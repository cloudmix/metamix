#include "clear_input.h"

#include "h264/stdseis.h"
#include "log.h"
#include "scte35/scte35.h"

using metamix::h264::build_cc_reset_metadata;
using metamix::scte35::SpliceInfoSection;
using metamix::scte35::SpliceNull;

namespace metamix {

std::optional<std::vector<Metadata<SeiKind>>>
ClearInput::run_query_sei(ClockTS since_ts, ClockTS until_ts, const ApplicationContext &)
{
  auto meta = build_cc_reset_metadata(m_spec.id, std::max(since_ts, until_ts - 1_clock));
  return std::make_optional<std::vector<Metadata<SeiKind>>>({ std::move(meta) });
}

std::optional<std::vector<Metadata<ScteKind>>>
ClearInput::run_query_scte(ClockTS since_ts, ClockTS until_ts, const ApplicationContext &)
{
  auto ts = std::max(since_ts, until_ts - 1_clock);
  auto br = std::make_shared<SpliceInfoSection>(false, 0, 0, 0, 0xfff, SpliceNull{});
  Metadata<ScteKind> meta(m_spec.id, ts, ts, 0, std::move(br));
  return std::make_optional<std::vector<Metadata<ScteKind>>>({ std::move(meta) });
}
}
