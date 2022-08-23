#include "ffmpeg.h"

std::runtime_error
ff::runtime_error(const std::string &description, int e)
{
  std::string errbuf(AV_ERROR_MAX_STRING_SIZE, '\0');
  av_strerror(e, errbuf.data(), errbuf.size());
  return std::runtime_error(description + ": " + errbuf);
}

std::runtime_error
ff::runtime_error(const char *description, int e)
{
  return ff::runtime_error(std::string(description), e);
}

void
ff::AVFormatContextDeleter::operator()(AVFormatContext *o) const
{
  avformat_close_input(&o);
  avformat_free_context(o);
}

void
ff::AVCodecContextDeleter::operator()(AVCodecContext *o) const
{
  avcodec_free_context(&o);
}

void
ff::AVPacketDeleter::operator()(AVPacket *o) const
{
  av_packet_free(&o);
}

ff::AVPacketUniquePtr
ff::packet_alloc()
{
  AVPacket *packet = av_packet_alloc();
  if (!packet) {
    throw std::runtime_error("Could not allocate memory for AVPacket");
  }
  return ff::AVPacketUniquePtr(packet);
}

void
ff::grow_packet(AVPacket &pkt, int grow_by)
{
  auto e = av_grow_packet(&pkt, grow_by);
  if (e != 0) {
    throw ff::runtime_error("Could not grow AVPacket", e);
  }
}

void
ff::shrink_packet(AVPacket &pkt, int size)
{
  av_shrink_packet(&pkt, size);
}

ff::AVPacketUnrefGuard::AVPacketUnrefGuard(AVPacket *packet)
  : m_pkt(packet)
{}

ff::AVPacketUnrefGuard::AVPacketUnrefGuard(ff::AVPacketUniquePtr &packet)
  : m_pkt(packet.get())
{}

ff::AVPacketUnrefGuard::~AVPacketUnrefGuard()
{
  av_packet_unref(m_pkt);
}

ff::AVPacketRef::AVPacketRef()
{
  av_init_packet(&m_packet);
}

ff::AVPacketRef::AVPacketRef(const AVPacket &packet)
  : AVPacketRef(&packet)
{}

ff::AVPacketRef::AVPacketRef(const AVPacket *packet)
  : m_packet{}
{
  auto e = av_packet_ref(&m_packet, packet);
  if (e < 0) {
    throw ff::runtime_error("Could not set up new AVPacket reference", e);
  }
}

ff::AVPacketRef::AVPacketRef(const ff::AVPacketRef &other)
  : m_packet{}
{
  if (av_packet_ref(&this->m_packet, &other.m_packet) < 0) {
    throw std::runtime_error("Could not set up new AVPacket reference");
  }
}

ff::AVPacketRef &
ff::AVPacketRef::operator=(const ff::AVPacketRef &rhs)
{
  if (this != &rhs) {
    av_packet_unref(&m_packet);

    auto e = av_packet_ref(&this->m_packet, &rhs.m_packet);
    if (e < 0) {
      throw ff::runtime_error("Could not set up new AVPacket reference", e);
    }
  }
  return *this;
}

ff::AVPacketRef &
ff::AVPacketRef::operator=(ff::AVPacketRef &&rhs) noexcept(false)
{
  if (this != &rhs) {
    AVPacket old_packet{};
    std::swap(old_packet, m_packet);

    auto e = av_packet_ref(&m_packet, &rhs.m_packet);
    if (e < 0) {
      throw ff::runtime_error("Could not set up new AVPacket reference", e);
    }

    av_packet_unref(&old_packet);
    av_packet_unref(&rhs.m_packet);
  }
  return *this;
}

ff::AVPacketRef::~AVPacketRef()
{
  av_packet_unref(&m_packet);
}
