#pragma once

#include <atomic>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

#include "application_context.h"
#include "clock_types.h"
#include "iospec.h"
#include "log.h"
#include "metadata.h"

namespace metamix {

/**
 * @brief Declares capabilities of given input to produce metadata of each kind.
 *
 * The contract is that if the input declares capability for kind A, than it overrides corresponding methods; otherwise
 * it leaves default implementations.
 */
struct InputCapabilities
{
  using Kinds = MetadataKindPack;

  bool sei{ false };
  bool scte{ false };

  constexpr InputCapabilities() = default;

  constexpr InputCapabilities(bool sei, bool scte)
    : sei(sei)
    , scte(scte)
  {}

  template<class K>
  inline constexpr bool has() const
  {
    return K::get(*this);
  }

  template<class K>
  inline void set(bool value)
  {
    K::get(*this) = value;
  }

  template<class K>
  void require() const
  {
    if (!has<K>()) {
      throw std::runtime_error(std::string("Required input capability has not been found: ") + K::DESCRIPTION);
    }
  }

  friend std::ostream &operator<<(std::ostream &os, const InputCapabilities &c)
  {
    return os << "SEI:" << c.sei << ", "
              << "SCTE:" << c.scte;
  }
};

class AbstractInput
{
private:
  InputCapabilities m_caps{};

public:
  constexpr AbstractInput() = default;

protected:
  constexpr AbstractInput(InputCapabilities caps)
    : m_caps{ std::move(caps) }
  {}

public:
  virtual ~AbstractInput() = default;

  virtual const InputSpec &spec() const = 0;

  InputCapabilities caps() const { return m_caps; }

  virtual void on_exit() {}

  virtual void schedule_restart() {}

  template<class K>
  inline std::vector<Metadata<K>> query(ClockTS since_ts, ClockTS until_ts, const ApplicationContext &ctx)
  {
    if (auto r = KindFuncs<K>::run_query(*this, since_ts, until_ts, ctx); r) {
      LOG(trace) << "Found " << r->size() << " " << K::NAME << " at pts [" << since_ts << ", " << until_ts << ") "
                 << ':' << spec().name;
      return *r;
    } else {
      LOG(trace) << "No " << K::NAME << " at pts [" << since_ts << ", " << until_ts << ") :" << spec().name;
      return KindFuncs<K>::build_empty_metadata(*this, since_ts, until_ts);
    }
  }

protected:
  virtual std::optional<std::vector<Metadata<SeiKind>>> run_query_sei(ClockTS since_ts,
                                                                      ClockTS until_ts,
                                                                      const ApplicationContext &ctx);

  virtual std::optional<std::vector<Metadata<ScteKind>>> run_query_scte(ClockTS since_ts,
                                                                        ClockTS until_ts,
                                                                        const ApplicationContext &ctx);

  template<class K>
  void declare_capability()
  {
    m_caps.set<K>(true);
  }

private:
  std::vector<Metadata<SeiKind>> build_empty_sei(ClockTS since_ts, ClockTS until_ts);
  std::vector<Metadata<ScteKind>> build_empty_scte(ClockTS since_ts, ClockTS until_ts);

  template<class K>
  struct KindFuncs
  {};
};

#define ALIAS_METHOD(ALIAS_NAME, METHOD_NAME)                                                                          \
  template<typename... Args>                                                                                           \
  inline static auto ALIAS_NAME(AbstractInput &input, Args &&... args)                                                 \
    ->decltype(input.METHOD_NAME(std::forward<Args>(args)...))                                                         \
  {                                                                                                                    \
    return input.METHOD_NAME(std::forward<Args>(args)...);                                                             \
  }

template<>
struct AbstractInput::KindFuncs<SeiKind>
{
  ALIAS_METHOD(run_query, run_query_sei)
  ALIAS_METHOD(build_empty_metadata, build_empty_sei)
};

template<>
struct AbstractInput::KindFuncs<ScteKind>
{
  ALIAS_METHOD(run_query, run_query_scte)
  ALIAS_METHOD(build_empty_metadata, build_empty_scte)
};

#undef ALIAS_METHOD
}
