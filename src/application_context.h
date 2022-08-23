#pragma once

#include <boost/signals2.hpp>

#include "metadata_queue.h"

namespace metamix {

class Clock;

class InputManager;

class ProgramOptions;

using ApplicationMetadataQueueGroup = MetadataKindPack::Apply<MetadataQueueGroup>;

class ApplicationContext
{
public:
  std::shared_ptr<ApplicationMetadataQueueGroup> meta_queue;
  std::shared_ptr<Clock> clock;
  std::shared_ptr<InputManager> input_manager;
  std::shared_ptr<const ProgramOptions> options;

  boost::signals2::signal<void()> on_exit{};

private:
  std::atomic<bool> m_running{ true };
  std::atomic<int64_t> m_ts_adjustment;

public:
  ApplicationContext(std::shared_ptr<ApplicationMetadataQueueGroup> meta_queue,
                     std::shared_ptr<Clock> clock,
                     std::shared_ptr<InputManager> input_manager,
                     std::shared_ptr<const ProgramOptions> options);

  bool is_running() const volatile noexcept { return m_running; }

  void exit();

  ClockTS ts_adjustment() const volatile noexcept { return ClockTS(m_ts_adjustment.load()); }

  void ts_adjustment(ClockTS value);
};
}
