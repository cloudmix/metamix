#include "source_handle.h"

#include "../log.h"

namespace metamix::io {

SourceHandle::SourceHandle(std::string name, std::string url)
  : IOHandle(std::move(name), std::move(url))
{}

void
SourceHandle::open(const std::optional<std::string> &format_name)
{
  int e;

  auto ctx = avformat_alloc_context();
  if (ctx == nullptr) {
    throw std::runtime_error("Could not allocate memory for format context");
  }

  LOG(info) << "Opening source stream " << url();

  AVInputFormat *input_format = nullptr;
  if (format_name) {
    input_format = av_find_input_format(format_name->c_str());
    if (input_format == nullptr) {
      throw std::runtime_error("Unknown input format " + *format_name);
    }
  }

  e = avformat_open_input(&ctx, url().c_str(), input_format, nullptr);
  if (e < 0) {
    throw ff::runtime_error("Could not open source stream", e);
  }

  LOG(debug) << "Finding stream info";
  e = avformat_find_stream_info(ctx, nullptr);
  if (e < 0) {
    throw ff::runtime_error("Could not find stream info", e);
  }

  av_dump_format(ctx, 0, url().c_str(), false);

  fmt_ctx = ff::AVFormatContextUniquePtr(ctx);
}

bool
SourceHandle::read_packet(AVPacket &packet)
{
  auto e = av_read_frame(fmt_ctx.get(), &packet);
  if (e >= 0 || e == AVERROR_EOF) {
    return e == 0;
  } else {
    throw ff::runtime_error("failed reading packet", e);
  }
}
}
