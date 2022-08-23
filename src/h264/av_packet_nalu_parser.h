#pragma once

#include <utility>

#include "../binary_parser.h"
#include "../ffmpeg.h"

#include "av_packet_nalu.h"
#include "nalu_parser.h"

namespace metamix::h264 {

struct AVPacketNaluParserContext
{
  ff::AVPacketRef packet;

  explicit AVPacketNaluParserContext(const AVPacket &packet)
    : packet(packet)
  {}
  explicit AVPacketNaluParserContext(ff::AVPacketRef packet)
    : packet(std::move(packet))
  {}

  inline const uint8_t *startptr() const { return packet->data; }

  inline const uint8_t *endptr() const { return packet->data + packet->size; }
};

inline AVPacketNalu
av_packet_nalu_parser_pack(const AVPacketNaluParserContext &ctx, const BinaryParserBounds &bounds)
{
  assert(bounds.startptr() >= ctx.startptr());
  assert(bounds.length() > 0);
  size_t offset = bounds.startptr() - ctx.startptr();
  return AVPacketNalu(&*ctx.packet, offset, bounds.length());
}

using AVPacketNaluParser = BinaryParser<AVPacketNalu,
                                        AVPacketNaluParserContext,
                                        BinaryParserBounds,
                                        nalu_parser_next,
                                        av_packet_nalu_parser_pack>;
}
