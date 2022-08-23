#include "sink_handle.h"

#include <boost/format.hpp>

#include "../log.h"
#include "../util.h"

namespace metamix::io {

SinkHandle::~SinkHandle()
{
  if (should_write_trailer) {
    assert(fmt_ctx);
    LOG(debug) << "Writing trailer in sink";
    av_write_trailer(fmt_ctx.get());
    should_write_trailer = false;
  }
}

void
SinkHandle::open(const std::optional<std::string> &format_name)
{
  int e;

  AVFormatContext *ctx{ nullptr };

  LOG(info) << "Opening sink stream " << url();

  const char *output_format_name = nullptr;
  if (format_name) {
    output_format_name = format_name->c_str();
  }

  e = avformat_alloc_output_context2(&ctx, nullptr, output_format_name, url().c_str());
  if (e < 0) {
    throw ff::runtime_error("Could not open sink stream", e);
  }

  if (!(ctx->oformat->flags & AVFMT_NOFILE)) {
    LOG(debug) << "Opening sink stream file";
    e = avio_open(&ctx->pb, url().c_str(), AVIO_FLAG_WRITE);
    if (e < 0) {
      throw ff::runtime_error((boost::format("Could not open sink %1%") % url()).str(), e);
    }
  }

  fmt_ctx = ff::AVFormatContextUniquePtr(ctx);
}

void
SinkHandle::init_remuxing(const SourceHandle &source)
{
  int e;

  LOG(debug) << "Copying streams from source to sink";

  auto input_streams = source.all_streams();
  for (auto in_stream : input_streams) {
    LOG(trace) << "Copying stream #" << in_stream->id;

    auto codecpar = in_stream->codecpar;

    auto out_stream = avformat_new_stream(fmt_ctx.get(), nullptr);
    if (!out_stream) {
      throw std::runtime_error("Failed allocating sink stream");
    }

    e = avcodec_parameters_copy(out_stream->codecpar, codecpar);
    if (e < 0) {
      throw ff::runtime_error("Failed to set up sink codec parameters", e);
    }

    out_stream->codecpar->codec_tag = 0;
  }

  av_dump_format(fmt_ctx.get(), 0, url().c_str(), true);
}

void
SinkHandle::start()
{
  int e;

  LOG(debug) << "Writing header in sink";

  e = avformat_write_header(fmt_ctx.get(), nullptr);
  if (e < 0) {
    throw ff::runtime_error("Could not write header in sink", e);
  }

  should_write_trailer = true;
}

void
SinkHandle::interleaved_write_frame(AVPacket &packet)
{
  int e = av_interleaved_write_frame(fmt_ctx.get(), &packet);
  if (e < 0) {
    throw ff::runtime_error("Error muxing packet", e);
  }
}

void
SinkHandle::remux_packet(AVPacket &pkt, const SourceHandle &source)
{
  auto in_stream = source.get_stream(pkt.stream_index);
  auto out_stream = get_stream(pkt.stream_index);

  av_packet_rescale_ts(&pkt, in_stream.time_base, out_stream.time_base);
  pkt.pos = -1;

  interleaved_write_frame(pkt);
}
}
