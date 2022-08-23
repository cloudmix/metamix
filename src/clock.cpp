#include "clock.h"

#include "ffmpeg.h"
#include "log.h"

namespace metamix {

TS
rescale_ts(TS ts, TimeBase intb, TimeBase outtb)
{
  return av_rescale_q(ts, sys_time_base_to_av(intb), sys_time_base_to_av(outtb));
}

std::ostream &
operator<<(std::ostream &os, const Clock &c)
{
  return os << "Clock{"
            << "mono=" << c.m_val << "}";
}

void
TSTicker::tick(ClockTS ts)
{
  if (!m_last_ts) {
    *m_clock += ts;
    m_last_ts = ts;
    return;
  }

  *m_clock += ts - *m_last_ts;
  m_last_ts = ts;
}

ClockTS
TSRescaler::rescale_to_clock(StreamTS ts)
{
  if (!m_ts_zero) {
    m_ts_zero = ts;
  }

  auto r = m_base + rescale_ts<StreamTS, ClockTS>(ts - *m_ts_zero, m_local_time_base, m_clock->time_base());

  // LOG(trace) << "ts: " << ts << ", "
  //            << "m_local_time_base: " << m_local_time_base << ", "
  //            << "m_ts_zero: " << *m_ts_zero << ", "
  //            << "m_base: " << m_base << ", "
  //            << "clock_time_base: " << m_clock->time_base() << ", "
  //            << "r: " << r;

  return r;
}
}
