#include "injector.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <iostream>
#include <iterator>
#include <thread>

#include "../application_context.h"
#include "../clock.h"
#include "../ffmpeg.h"
#include "../h264/av_packet_nalu.h"
#include "../h264/av_packet_nalu_parser.h"
#include "../h264/emitter.h"
#include "../h264/rbsp.h"
#include "../h264/sei_parser.h"
#include "../h264/sei_payload.h"
#include "../h264/stdseis.h"
#include "../input_manager.h"
#include "../io/remux_loop.h"
#include "../io/sink_handle.h"
#include "../io/source_handle.h"
#include "../iospec.h"
#include "../log.h"
#include "../metadata.h"
#include "../program_options.h"
#include "../util.h"

using metamix::ScteKind;
using metamix::SeiKind;
using metamix::TimeSourceKind;
using metamix::h264::AVPacketNalu;
using metamix::h264::AVPacketNaluParser;
using metamix::h264::build_cc_reset_metadata;
using metamix::h264::copy_ebsp_to_sodb;
using metamix::h264::emit_sei_payloads_to_avcc_nalu;
using metamix::h264::NaluType;
using metamix::h264::OwnedSeiPayload;
using metamix::h264::SeiParser;
using metamix::h264::SeiType;
using metamix::io::NullPacketProcessor;
using metamix::io::PacketProcessor;
using metamix::io::SinkHandle;
using metamix::io::SourceHandle;
using namespace std::chrono_literals;
namespace ph = std::placeholders;

namespace metamix::proc {

template<class T>
static void
vector_append_move(T &vec, T app)
{
  vec.insert(std::end(vec), std::make_move_iterator(std::begin(app)), std::make_move_iterator(std::end(app)));
}

namespace {

class ClockTicker : public PacketProcessor<TimeSourceKind>
{
private:
  TSTicker ticker;
  TSRescaler pts_rescaler;

public:
  ClockTicker(StreamTimeBase stream_time_base, const ApplicationContext &ctx)
    : ticker{ ctx.clock }
    , pts_rescaler{ TSRescaler::clock_relative(ctx.clock, stream_time_base) }
  {}

  static std::unique_ptr<PacketProcessor<TimeSourceKind>> factory(StreamTimeBase stream_time_base,
                                                                  const ApplicationContext &ctx)
  {
    return std::make_unique<ClockTicker>(stream_time_base, ctx);
  }

  bool process(AVPacket &pkt) override
  {
    auto rescaled_pts = pts_rescaler.rescale_to_clock(StreamTS(pkt.pts));
    ticker.tick(rescaled_pts);
    return false;
  }
};

class SeiInjector : public PacketProcessor<SeiKind>
{
private:
  const ApplicationContext &ctx;

  TSRescaler pts_rescaler;

  ClockTS prev_pts{ std::numeric_limits<TS>::min() };
  std::optional<InputId> prev_input_id = std::nullopt;

public:
  SeiInjector(StreamTimeBase stream_time_base, const ApplicationContext &ctx)
    : ctx{ ctx }
    , pts_rescaler{ TSRescaler::clock_relative(ctx.clock, stream_time_base) }
  {}

  static std::unique_ptr<PacketProcessor<SeiKind>> factory(StreamTimeBase stream_time_base,
                                                           const ApplicationContext &ctx)
  {
    return std::make_unique<SeiInjector>(stream_time_base, ctx);
  }

  bool process(AVPacket &pkt) override
  {
    // LOG(trace) << "dts: " << pkt.dts << " pts: " << pkt.pts << " pos: " << pkt.pos << " dur: " << pkt.duration
    //            << " flags: 0x" << std::hex << pkt.flags;

    std::vector<Metadata<SeiKind>> found_sei_metadata{};
    std::vector<uint8_t> buf{};

    auto rescaled_pts = pts_rescaler.rescale_to_clock(StreamTS(pkt.pts)) - ctx.ts_adjustment();

    auto &input = ctx.input_manager->get_current_input<SeiKind>();
    const auto input_id = input.spec().id;

    if (input_id != prev_input_id) {
      found_sei_metadata.push_back(build_cc_reset_metadata(input_id, rescaled_pts));
    }

    prev_input_id = input_id;

    vector_append_move(found_sei_metadata, input.query<SeiKind>(prev_pts, rescaled_pts + 1_clock, ctx));

    prev_pts = rescaled_pts + 1_clock;

    try {
      // Collect all NALUs from packet
      std::vector<AVPacketNalu> nalus;
      auto nalu_parser = AVPacketNaluParser::create(pkt);
      while (nalu_parser) {
        AVPacketNalu nalu;
        nalu_parser >> nalu;

        if (!nalu.is_valid()) {
          LOG(warning) << "Invalid NALU spotted";
          continue;
        }

        nalus.push_back(nalu);
      }

      auto it = nalus.begin();

      // Remux NALUs which are expected to be first, before SEIs
      for (; it != nalus.end(); it++) {
        if (it->type() == NaluType::AUD || it->type() == NaluType::SPS || it->type() == NaluType::PPS) {
          emit_avcc_nalu(*it, std::back_inserter(buf));
        } else {
          break;
        }
      }

      // Get first SEI NALU and collect its payloads, stripping existing closed captions
      std::vector<OwnedSeiPayload> seis{};
      if (it != nalus.end() && it->type() == NaluType::SEI) {
        seis = strip_cc(*it);
        it++;
      }

      // Append closed captions from metadata queue
      for (const auto &meta : found_sei_metadata) {
        seis.push_back(*meta.val);
      }

      // Remux SEI NALU
      assert(!seis.empty());
      emit_sei_payloads_to_avcc_nalu(seis.begin(), seis.end(), std::back_inserter(buf));

      // Remux rest of packets
      for (; it != nalus.end(); it++) {
        emit_avcc_nalu(*it, std::back_inserter(buf));
      }

      // Adjust remuxed packet size; beware of FFmpeg API inconsistency regarding second argument!
      if (pkt.size < buf.size()) {
        // If original packet is smaller than remuxed one, grow it by size difference
        ff::grow_packet(pkt, buf.size() - pkt.size);
      } else if (pkt.size > buf.size()) {
        // If original packet is bigger than remuxed one, set its size to remuxed one's size
        ff::shrink_packet(pkt, buf.size());
      }

      assert(pkt.size == buf.size());

      std::copy(buf.begin(), buf.end(), pkt.data);
    } catch (BinaryParseError &ex) {
      LOG(error) << "Parse error: " << ex;
    }

    return false;
  }

private:
  std::vector<OwnedSeiPayload> strip_cc(const AVPacketNalu &nalu)
  {
    assert(nalu.type() == NaluType::SEI);

    std::vector<OwnedSeiPayload> non_cc_seis;

    try {
      std::vector<uint8_t> sodb{};
      sodb.reserve(nalu.size());
      copy_ebsp_to_sodb(nalu.data(), nalu.data() + nalu.size(), std::back_inserter(sodb));

      SeiParser sei_parser = SeiParser::create(sodb.data() + 1, sodb.data() + sodb.size());
      OwnedSeiPayload sei;

      while (sei_parser) {
        sei_parser >> sei;

        if (sei.type() == SeiType::USER_DATA_REGISTERED) {
          LOG(trace) << "Dropping CC SEI from source.";
          // metamix::hex_dump(sei.begin(), sei.end());
        } else {
          non_cc_seis.push_back(std::move(sei));
        }
      }
    } catch (BinaryParseError &) {
      LOG(error) << "Error stripping CC SEI...";
      throw;
    }

    return non_cc_seis;
  }
};
}

void
injector(std::shared_ptr<ApplicationContext> ctx)
{
  metamix::log::set_thread_name("output");

  auto output_spec = ctx->options->output;

  SinkHandle sink("output", output_spec.sink);
  SourceHandle source("output", output_spec.source);

  source.open(output_spec.source_format);
  sink.open(output_spec.sink_format);

  sink.init_remuxing(source);

  const auto sc = source.classify_streams();

  sc.require<TimeSourceKind>();

  sink.start();

  // LOG(warning) << "Press ENTER to start injecting!";
  // std::string tmp;
  // std::getline(std::cin, tmp);

  remux_loop(source,
             sink,
             sc,
             { std::bind(&ClockTicker::factory, ph::_1, std::cref(*ctx)),
               std::bind(&SeiInjector::factory, ph::_1, std::cref(*ctx)),
               NullPacketProcessor<ScteKind>::factory });
}
}
