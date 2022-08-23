#include "remux_loop.h"

#include <cassert>
#include <type_traits>

#include <boost/format.hpp>

#include "../clock_types.h"
#include "../ffmpeg.h"
#include "../log.h"

namespace metamix::io {

void
remux_loop(SourceHandle &source,
           SinkHandle &sink,
           const StreamClassification &sc,
           PacketProcessorFactoryGroup factories)
{
  constexpr int MAX_RETRY = 10;

  static auto NON_MONO_DTS = boost::format("Input provided invalid, non monotonically increasing dts to "
                                           "muxer in stream %1%: %2% >= %3%. "
                                           "This is interpreted as stream being restarted.");
  static auto PTS_LT_DTS = boost::format("pts (%2%) < dts (%3%) in stream %1%. "
                                         "This is interpreted as stream being restarted.");

  auto mkproc = [&](auto &factory) {
    using Kind = typename std::decay_t<decltype(factory)>::result_type::element_type::Kind;

    if (sc.has<Kind>()) {
      auto stream_time_base = av_time_base_to_sys<StreamTimeBase>(source.get_stream<TimeSourceKind>(sc).time_base);
      return factory(stream_time_base);
    } else {
      return std::unique_ptr<PacketProcessor<Kind>>{};
    }
  };

  auto procs = std::apply([&](auto &... f) { return std::make_tuple(mkproc(f)...); }, factories);

  int read_trial = 0;
  int remux_trial = 0;
  auto pkt = ff::packet_alloc();

  for (;;) {
    try {
      auto e = source.read_packet(*pkt);
      read_trial = 0;
      if (!e) {
        break;
      }
    } catch (std::runtime_error &ex) {
      if (read_trial < MAX_RETRY) {
        read_trial++;
        LOG(error) << ex.what();
        LOG(trace) << "Read trial: " << read_trial;
        continue;
      } else {
        throw;
      }
    }

    if (auto st = sink.get_stream(pkt->stream_index);
        st.cur_dts && st.cur_dts != AV_NOPTS_VALUE &&
        // clang-format off
        ((!(sink.oformat().flags & AVFMT_TS_NONSTRICT) &&
          st.codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE &&
          st.codecpar->codec_type != AVMEDIA_TYPE_DATA &&
          st.cur_dts >= pkt->dts) || st.cur_dts > pkt->dts)
        // clang-format on
    ) {
      throw std::runtime_error((NON_MONO_DTS % pkt->stream_index % st.cur_dts % pkt->dts).str());
    }

    if (pkt->dts != AV_NOPTS_VALUE && pkt->pts != AV_NOPTS_VALUE && pkt->pts < pkt->dts) {
      throw std::runtime_error((PTS_LT_DTS % pkt->stream_index % pkt->pts % pkt->dts).str());
    }

    ff::AVPacketUnrefGuard packet_guard(pkt);

    bool brk = false;

    auto procf = [&](auto &proc) {
      using Kind = typename std::decay_t<decltype(*proc)>::Kind;

      if (pkt->stream_index == sc.index<Kind>()) {
        assert(proc != nullptr);

        bool my_brk = proc->process(*pkt);

        if (my_brk) {
          LOG(debug) << "The processor of '" << Kind::DESCRIPTION << "' requested a user-break";
          brk = true;
        }
      }
    };

    std::apply([&](auto &... f) { (..., procf(f)); }, procs);

    try {
      sink.remux_packet(*pkt, source);
      remux_trial = 0;
    } catch (std::runtime_error &ex) {
      if (remux_trial < MAX_RETRY) {
        remux_trial++;
        LOG(error) << ex.what();
        LOG(trace) << "Remux trial: " << remux_trial;
      } else {
        throw;
      }
    }

    if (brk) {
      return;
    }
  }

  LOG(warning) << "EOF";
}
}
