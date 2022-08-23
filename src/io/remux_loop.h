#pragma once

#include <functional>
#include <memory>
#include <tuple>

#include "../clock_types.h"

#include "sink_handle.h"
#include "source_handle.h"

namespace metamix::io {

template<class K>
class PacketProcessor
{
public:
  using Kind = K;
  using Factory = std::function<std::unique_ptr<PacketProcessor<K>>(StreamTimeBase)>;

public:
  constexpr PacketProcessor() = default;

  virtual ~PacketProcessor() {}

  virtual bool process(AVPacket &pkt) = 0;
};

template<class K>
struct NullPacketProcessor : public PacketProcessor<K>
{
  constexpr NullPacketProcessor() = default;

  static std::unique_ptr<PacketProcessor<K>> factory([[maybe_unused]] StreamTimeBase stb)
  {
    return std::make_unique<NullPacketProcessor<K>>();
  }

  bool process(AVPacket &) override { return false; }
};

namespace detail {

template<typename P>
using GetPacketProcessorFactory = typename P::Factory;
}

using PacketProcessorFactoryGroup =
  StreamClassPack::Map<PacketProcessor>::Map<detail::GetPacketProcessorFactory>::Apply<std::tuple>;

void
remux_loop(SourceHandle &source,
           SinkHandle &sink,
           const StreamClassification &sc,
           PacketProcessorFactoryGroup factories);
}
