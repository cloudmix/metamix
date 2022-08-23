#pragma once

#include <atomic>

#include "abstract_input.h"

namespace metamix {

class UserDefinedInput : public AbstractInput
{
private:
  InputSpec m_spec;
  std::atomic<bool> m_restart_scheduled{ false };

public:
  explicit UserDefinedInput(InputSpec spec)
    : m_spec(std::move(spec))
  {}

  ~UserDefinedInput() override = default;

  const InputSpec &spec() const override { return m_spec; };

  bool is_restart_scheduled() volatile { return m_restart_scheduled; }

  void is_restart_scheduled(bool scheduled) volatile { m_restart_scheduled = scheduled; }

  void schedule_restart() override;

  template<class K>
  void push(ClockTS pts,
            ClockTS dts,
            int order,
            std::shared_ptr<typename Metadata<K>::ValueType> val,
            const ApplicationContext &ctx)
  {
    declare_capability<K>();
    ctx.meta_queue->push(Metadata<K>(spec().id, pts, dts, order, std::move(val)));
  }

protected:
  virtual std::optional<std::vector<Metadata<SeiKind>>> run_query_sei(ClockTS since_ts,
                                                                      ClockTS until_ts,
                                                                      const ApplicationContext &ctx);
};
}
