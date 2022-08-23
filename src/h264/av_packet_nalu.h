#pragma once

#include "nalu.h"

#include "../ffmpeg.h"

namespace metamix::h264 {

class AVPacketNalu : public Nalu
{
private:
  ff::AVPacketRef m_packet{};
  size_t m_offset{ 0 };
  size_t m_length{ 0 };

public:
  AVPacketNalu() = default;
  AVPacketNalu(const AVPacketNalu &nalu) = default;
  AVPacketNalu(AVPacketNalu &&nalu) noexcept = default;

  AVPacketNalu(const AVPacket *packet, size_t offset, size_t length)
    : m_packet{ packet }
    , m_offset{ offset }
    , m_length{ length }
  {}

  ~AVPacketNalu() override = default;

  AVPacketNalu &operator=(const AVPacketNalu &rhs) = default;
  AVPacketNalu &operator=(AVPacketNalu &&rhs) noexcept(false) = default;

  size_t size() const noexcept override { return m_length; }

  uint8_t *data() noexcept override { return m_packet->data + m_offset; }

  const uint8_t *data() const noexcept override { return m_packet->data + m_offset; }
};
}
