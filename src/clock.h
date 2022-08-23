#include <utility>

#pragma once

#include <atomic>
#include <cassert>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <ostream>
#include <utility>

#include "clock_types.h"

namespace metamix {

TS
rescale_ts(TS ts, TimeBase intb, TimeBase outtb);

template<class From, class To>
inline To
rescale_ts(From ts, typename From::TimeBaseType intb, typename To::TimeBaseType outtb)
{
  return To(rescale_ts(ts.val, intb.val, outtb.val));
}

class Clock
{
private:
  std::atomic<TS> m_val{ 0 };
  ClockTimeBase m_my_time_base{ 1, SYS_CLOCK_RATE };

public:
  Clock() = default;

  explicit Clock(ClockTS initial_value)
    : m_val(initial_value)
  {}

  Clock(ClockTS initial_value, ClockTimeBase time_base)
    : m_val(initial_value)
    , m_my_time_base(time_base)
  {}

  ClockTS now() const volatile noexcept { return ClockTS(m_val); }

  /// Guaranteed to be constant during entire clock life time.
  constexpr ClockTimeBase time_base() const noexcept { return ClockTimeBase(m_my_time_base); }

  void increment(ClockTS delta) noexcept
  {
    if (delta >= 0_clock) {
      m_val += delta;
    }
  }

  Clock &operator+=(ClockTS delta) noexcept
  {
    increment(delta);
    return *this;
  }

  friend std::ostream &operator<<(std::ostream &os, const Clock &c);
};

class TSTicker
{
private:
  std::shared_ptr<Clock> m_clock;
  std::optional<ClockTS> m_last_ts{};

public:
  explicit TSTicker(std::shared_ptr<Clock> clock)
    : m_clock(std::move(clock))
  {}

  void tick(ClockTS ts);
};

class TSRescaler
{
private:
  std::shared_ptr<Clock> m_clock;
  StreamTimeBase m_local_time_base;
  ClockTS m_base;
  std::optional<StreamTS> m_ts_zero{};

public:
  TSRescaler(std::shared_ptr<Clock> clock, ClockTS base, StreamTimeBase local_time_base) noexcept
    : m_clock(std::move(clock))
    , m_local_time_base(local_time_base)
    , m_base(base)
  {}

  static TSRescaler clock_relative(std::shared_ptr<Clock> clock, StreamTimeBase local_time_base) noexcept
  {
    auto clock_delta = clock->now();
    return TSRescaler(std::move(clock), clock_delta, local_time_base);
  }

  ClockTS rescale_to_clock(StreamTS ts);
};
}
