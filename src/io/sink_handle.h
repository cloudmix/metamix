#pragma once

#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "../ffmpeg.h"
#include "../util.h"

#include "io_handle.h"
#include "source_handle.h"

namespace metamix::io {

class SinkHandle : public IOHandle
{
private:
  bool should_write_trailer{ false };

public:
  SinkHandle(std::string name, std::string url)
    : IOHandle(std::move(name), std::move(url))
  {}

  ~SinkHandle() override;

  SinkHandle(const SinkHandle &) = delete;
  SinkHandle &operator=(const SinkHandle &) = delete;

  SinkHandle(SinkHandle &&other) noexcept = default;
  SinkHandle &operator=(SinkHandle &&other) noexcept = default;

  AVOutputFormat &oformat() { return *NULL_PROTECT(fmt_ctx->oformat); }

  const AVOutputFormat &oformat() const { return *NULL_PROTECT(fmt_ctx->oformat); }

  void open(const std::optional<std::string> &format_name = std::nullopt);

  void init_remuxing(const SourceHandle &source);

  void start();

  void interleaved_write_frame(AVPacket &packet);

  void remux_packet(AVPacket &pkt, const SourceHandle &source);
};
}
