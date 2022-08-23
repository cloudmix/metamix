#pragma once

#include <cstdint>
#include <memory>
#include <ostream>

#include "clock_types.h"
#include "iospec.h"
#include "metadata_kind.h"

namespace metamix::detail {

template<class Kind>
struct MetadataKindToValueMapping
{};
}

#define METAMIX_METADATA_KIND_MAP_TO_VALUE(KIND, VALUE_TYPE)                                                           \
  namespace metamix::detail {                                                                                          \
  template<>                                                                                                           \
  struct MetadataKindToValueMapping<KIND>                                                                              \
  {                                                                                                                    \
    using ValueType = VALUE_TYPE;                                                                                      \
  };                                                                                                                   \
  }

namespace metamix::h264 {
class OwnedSeiPayload;
}

namespace metamix::scte35 {
class SpliceInfoSection;
}

METAMIX_METADATA_KIND_MAP_TO_VALUE(SeiKind, metamix::h264::OwnedSeiPayload)
METAMIX_METADATA_KIND_MAP_TO_VALUE(ScteKind, metamix::scte35::SpliceInfoSection)

namespace metamix {

using MetadataKindPack = KindPack<SeiKind, ScteKind>;

template<class K>
struct Metadata
{
  using Kind = K;
  using ValueType = typename detail::MetadataKindToValueMapping<K>::ValueType;
  using Self = Metadata<Kind>;

  InputId input_id;
  ClockTS pts;
  ClockTS dts;
  int order;
  std::shared_ptr<ValueType> val;

  Metadata(InputId input_id, ClockTS pts, ClockTS dts, int order, std::shared_ptr<ValueType> val)
    : input_id{ input_id }
    , pts{ pts }
    , dts{ dts }
    , order{ order }
    , val{ std::move(val) }
  {}

  Metadata(const Self &) = default;
  Self &operator=(const Self &) = default;

  Metadata(Self &&) noexcept = default;
  Self &operator=(Self &&) noexcept = default;

  bool operator==(const Self &rhs) const
  {
    return std::tie(input_id, pts, dts, order, *val) == std::tie(rhs.input_id, rhs.pts, rhs.dts, rhs.order, *rhs.val);
  }

  bool operator!=(const Self &rhs) const { return !(rhs == *this); }

  friend std::ostream &operator<<(std::ostream &os, const Self &msg)
  {
    return os << "Metadata<" << Kind() << ">{"
              << "input_id=" << msg.input_id << ","
              << "pts=" << msg.pts << ","
              << "dts=" << msg.dts << ","
              << "order=" << msg.order << ","
              << "val=" << *msg.val << "}";
  }
};
}
