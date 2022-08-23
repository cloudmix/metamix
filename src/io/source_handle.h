#pragma once

#include <iostream>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "../ffmpeg.h"
#include "../util.h"

#include "io_handle.h"

namespace metamix::io {

class SourceHandle : public IOHandle
{
public:
  SourceHandle(std::string name, std::string url);

  SourceHandle(const SourceHandle &) = delete;
  SourceHandle &operator=(const SourceHandle &) = delete;

  SourceHandle(SourceHandle &&other) noexcept = default;
  SourceHandle &operator=(SourceHandle &&other) noexcept = default;

  AVInputFormat &iformat() { return *NULL_PROTECT(fmt_ctx->iformat); }

  const AVInputFormat &iformat() const { return *NULL_PROTECT(fmt_ctx->iformat); }

  void open(const std::optional<std::string> &format_name = std::nullopt);

  bool read_packet(AVPacket &packet);
};
}
