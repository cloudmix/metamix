#pragma once

#include <optional>
#include <ostream>
#include <string>
#include <utility>

#include "../metadata_kind.h"

namespace metamix::io {

using StreamClassPack = KindPack<TimeSourceKind, SeiKind, ScteKind>;

struct StreamClassification
{
  using Kinds = StreamClassPack;

  std::optional<size_t> time_source{};
  std::optional<size_t> sei{};
  std::optional<size_t> scte{};

  template<class K>
  bool has() const
  {
    return K::get(*this).has_value();
  }

  template<class K>
  std::optional<size_t> index() const
  {
    return K::get(*this);
  }

  template<class K>
  void classify(size_t idx)
  {
    std::optional<size_t> &opt = K::get(*this);
    if (opt) {
      throw std::runtime_error(std::string("Stream kind already registered: ") + K::DESCRIPTION);
    } else {
      opt = idx;
    }
  }

  template<class K>
  void require() const
  {
    if (!has<K>()) {
      throw std::runtime_error(std::string("Required stream kind has not been found: ") + K::DESCRIPTION);
    }
  }

  friend std::ostream &operator<<(std::ostream &os, const StreamClassification &sc)
  {
    return os << "TimeSource:" << sc.time_source << ", "
              << "SEI:" << sc.sei << ", "
              << "SCTE:" << sc.scte;
  }
};
}
