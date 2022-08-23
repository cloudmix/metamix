#pragma once

#include "abstract_input.h"

namespace metamix {

class ClearInput : public AbstractInput
{
public:
  static constexpr const char *NAME = "clear";

private:
  InputSpec m_spec{};

public:
  explicit ClearInput(InputId input_id)
    : AbstractInput(InputCapabilities(true, true))
  {
    m_spec.id = input_id;
    m_spec.name = NAME;
    m_spec.is_virtual = true;
  }

  ~ClearInput() override = default;

  const InputSpec &spec() const override { return m_spec; };

protected:
  virtual std::optional<std::vector<Metadata<SeiKind>>> run_query_sei(ClockTS since_ts,
                                                                      ClockTS until_ts,
                                                                      const ApplicationContext &ctx);

  virtual std::optional<std::vector<Metadata<ScteKind>>> run_query_scte(ClockTS since_ts,
                                                                        ClockTS until_ts,
                                                                        const ApplicationContext &ctx);
};
}
