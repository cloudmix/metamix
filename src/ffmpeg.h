#pragma once

#include <memory>
#include <stdexcept>
#include <string>

#include <boost/rational.hpp>

#ifdef __cplusplus
extern "C"
{
#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/timestamp.h>

#ifdef __cplusplus
}
#endif

namespace ff {

std::runtime_error
runtime_error(const std::string &description, int e);

std::runtime_error
runtime_error(const char *description, int e);

struct AVFormatContextDeleter
{
  void operator()(AVFormatContext *o) const;
};

struct AVCodecContextDeleter
{
  void operator()(AVCodecContext *o) const;
};

struct AVPacketDeleter
{
  void operator()(AVPacket *o) const;
};

using AVFormatContextUniquePtr = std::unique_ptr<AVFormatContext, AVFormatContextDeleter>;
using AVCodecContextUniquePtr = std::unique_ptr<AVCodecContext, AVCodecContextDeleter>;
using AVPacketUniquePtr = std::unique_ptr<AVPacket, AVPacketDeleter>;

AVPacketUniquePtr
packet_alloc();

void
grow_packet(AVPacket &pkt, int grow_by);

void
shrink_packet(AVPacket &pkt, int size);

class AVPacketUnrefGuard
{
private:
  AVPacket *m_pkt = nullptr;

public:
  AVPacketUnrefGuard() = default;
  explicit AVPacketUnrefGuard(AVPacket *packet);
  explicit AVPacketUnrefGuard(AVPacketUniquePtr &packet);

  AVPacketUnrefGuard(const AVPacketUnrefGuard &) = delete;
  AVPacketUnrefGuard &operator=(const AVPacketUnrefGuard &) = delete;

  AVPacketUnrefGuard(AVPacketUnrefGuard &&other) noexcept = default;
  AVPacketUnrefGuard &operator=(AVPacketUnrefGuard &&other) noexcept = default;

  ~AVPacketUnrefGuard();
};

class AVPacketRef
{
private:
  AVPacket m_packet{};

public:
  AVPacketRef();
  explicit AVPacketRef(const AVPacket &packet);
  explicit AVPacketRef(const AVPacket *packet);

  AVPacketRef(const AVPacketRef &other);
  AVPacketRef &operator=(const AVPacketRef &rhs);

  AVPacketRef(AVPacketRef &&other) noexcept(false) = default;
  AVPacketRef &operator=(AVPacketRef &&rhs) noexcept(false);

  virtual ~AVPacketRef();

  AVPacket &operator*() noexcept { return m_packet; }

  const AVPacket &operator*() const noexcept { return m_packet; }

  AVPacket *operator->() noexcept { return &m_packet; }

  const AVPacket *operator->() const noexcept { return &m_packet; }
};
}

template<class TimeBase>
AVRational
sys_time_base_to_av(const TimeBase &rational)
{
  return av_make_q(rational.numerator(), rational.denominator());
}

template<class TimeBase>
TimeBase
av_time_base_to_sys(const AVRational &rational)
{
  return TimeBase(rational.num, rational.den);
}
