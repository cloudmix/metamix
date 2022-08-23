#include "io_handle.h"

#include <boost/algorithm/string.hpp>

#include "../log.h"

namespace metamix::io {

namespace {

const AVInputFormat *
find_iformat(const char *needle)
{
  const AVInputFormat *fmt = nullptr;
  void *opaque = nullptr;

  while ((fmt = av_demuxer_iterate(&opaque)) != nullptr) {
    for (auto it = boost::make_split_iterator(fmt->name, boost::first_finder(","));
         it != boost::split_iterator<const char *>();
         it++) {
      if (boost::copy_range<std::string>(*it) == needle) {
        return fmt;
      }
    }
  }

  return nullptr;
}

const AVOutputFormat *
find_oformat(const char *needle)
{
  const AVOutputFormat *fmt = nullptr;
  void *opaque = nullptr;

  while ((fmt = av_muxer_iterate(&opaque)) != nullptr) {
    for (auto it = boost::make_split_iterator(fmt->name, boost::first_finder(","));
         it != boost::split_iterator<const char *>();
         it++) {
      if (boost::copy_range<std::string>(*it) == needle) {
        return fmt;
      }
    }
  }

  return nullptr;
}
}

std::vector<const AVStream *>
IOHandle::all_streams() const
{
  return std::vector<const AVStream *>(fmt_ctx->streams, fmt_ctx->streams + fmt_ctx->nb_streams);
}

StreamClassification
IOHandle::classify_streams() const
{
  static const AVInputFormat *ifmt_mpegts = find_iformat("mpegts");
  static const AVOutputFormat *ofmt_mpegts = find_oformat("mpegts");

  if(ifmt_mpegts == nullptr || ofmt_mpegts == nullptr) {
    LOG(warning) << "Using FFmpeg build with no support for MPEG-TS!";
  }

  int e;
  StreamClassification sc;

  LOG(info) << "Classifying streams";
  for (unsigned int i = 0; i < fmt_ctx->nb_streams; ++i) {
    LOG(debug) << "Classifying stream " << i;

    AVCodecParameters *codec_parameters = fmt_ctx->streams[i]->codecpar;

    AVCodec *codec = avcodec_find_decoder(codec_parameters->codec_id);
    if (codec) {
      LOG(debug) << "Codec: " << codec->long_name;
    } else {
      LOG(debug) << "Unknown codec " << codec_parameters->codec_id;
    }

    if (codec_parameters->codec_id == AV_CODEC_ID_H264) {
      if ((ifmt_mpegts != nullptr && fmt_ctx->iformat != nullptr && fmt_ctx->iformat == ifmt_mpegts) ||
          (ofmt_mpegts != nullptr && fmt_ctx->oformat != nullptr && fmt_ctx->oformat == ofmt_mpegts)) {
        LOG(warning) << "Found H.264 stream within MPEG-TS, Annex B formatted NALUs are not supported yet";
      } else {
        LOG(debug) << "This is CC SEI stream";
        sc.classify<SeiKind>(i);
      }
    } else if (codec_parameters->codec_id == AV_CODEC_ID_SCTE_35) {
      LOG(debug) << "This is SCTE-35 stream";
      sc.classify<ScteKind>(i);
    } else {
      LOG(debug) << "This is not an interesting stream";
    }
  }

  if (sc.has<SeiKind>()) {
    LOG(debug) << "Classifying SEI stream as system time source.";
    sc.classify<TimeSourceKind>(*sc.index<SeiKind>());
  } else {
    e = av_find_best_stream(fmt_ctx.get(), AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (e >= 0) {
      LOG(debug) << "Classifying stream #" << e << " as system time source, as this is the best video stream.";
      sc.classify<TimeSourceKind>(e);
    } else {
      LOG(debug) << "No candidate stream for system time source.";
    }
  }

  LOG(debug) << sc;

  return sc;
}
}
