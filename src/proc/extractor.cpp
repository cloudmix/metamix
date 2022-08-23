#include "extractor.h"

#include <thread>

#include <boost/scope_exit.hpp>

#include "../clock.h"
#include "../ffmpeg.h"
#include "../h264/av_packet_nalu.h"
#include "../h264/av_packet_nalu_parser.h"
#include "../h264/rbsp.h"
#include "../h264/sei_parser.h"
#include "../h264/stdseis.h"
#include "../input_manager.h"
#include "../io/remux_loop.h"
#include "../io/sink_handle.h"
#include "../io/source_handle.h"
#include "../log.h"
#include "../metadata.h"
#include "../metadata_queue.h"
#include "../program_options.h"
#include "../scte35/parser.h"
#include "../scte35/scte35.h"
#include "../user_defined_input.h"
#include "../util.h"

using metamix::ScteKind;
using metamix::SeiKind;
using metamix::TimeSourceKind;
using metamix::h264::AVPacketNalu;
using metamix::h264::AVPacketNaluParser;
using metamix::h264::build_cc_reset_metadata;
using metamix::h264::copy_ebsp_to_sodb;
using metamix::h264::NaluType;
using metamix::h264::OwnedSeiPayload;
using metamix::h264::SeiParser;
using metamix::h264::SeiType;
using metamix::io::PacketProcessor;
using metamix::io::SinkHandle;
using metamix::io::SourceHandle;
using metamix::scte35::Scte35Parser;
using metamix::scte35::SpliceInfoSection;
namespace ph = std::placeholders;

namespace metamix::proc {

namespace {

class MaintenanceProcessor : public PacketProcessor<TimeSourceKind>
{
private:
  UserDefinedInput &input;

public:
  MaintenanceProcessor(UserDefinedInput &input)
    : input{ input }
  {}

  static std::unique_ptr<PacketProcessor<TimeSourceKind>> factory(UserDefinedInput &input)
  {
    return std::make_unique<MaintenanceProcessor>(input);
  }

  bool process([[maybe_unused]] AVPacket &pkt) override { return input.is_restart_scheduled(); }
};

class SeiExtractor : public PacketProcessor<SeiKind>
{
private:
  UserDefinedInput &input;
  const ApplicationContext &ctx;

  TSRescaler pts_rescaler;
  TSRescaler dts_rescaler;

public:
  SeiExtractor(StreamTimeBase stream_time_base, UserDefinedInput &input, const ApplicationContext &ctx)
    : input{ input }
    , ctx{ ctx }
    , pts_rescaler{ TSRescaler::clock_relative(ctx.clock, stream_time_base) }
    , dts_rescaler{ TSRescaler::clock_relative(ctx.clock, stream_time_base) }
  {}

  static std::unique_ptr<PacketProcessor<SeiKind>> factory(StreamTimeBase stream_time_base,
                                                           UserDefinedInput &input,
                                                           const ApplicationContext &ctx)
  {
    return std::make_unique<SeiExtractor>(stream_time_base, input, ctx);
  }

  bool process(AVPacket &pkt) override
  {
    int order = 0;

    // LOG(trace) << "dts: " << pkt.dts << " pts: " << pkt.pts << " pos: " << pkt.pos << " dur: " << pkt.duration
    //            << " flags: 0x" << std::hex << pkt.flags;

    try {
      auto parser = AVPacketNaluParser::create(pkt);
      AVPacketNalu nalu;
      while (parser) {
        parser >> nalu;

        if (!nalu.is_valid()) {
          LOG(error) << "Invalid NALU, skipping processing";
        } else if (nalu.type() == NaluType::SEI) {
          std::vector<uint8_t> sodb_data;
          sodb_data.reserve(nalu.size());

          copy_ebsp_to_sodb(nalu.data(), nalu.data() + nalu.size(), std::back_inserter(sodb_data));

          SeiParser sei_parser = SeiParser::create(sodb_data.data() + 1, sodb_data.data() + sodb_data.size());
          while (sei_parser) {
            auto sei = std::make_shared<OwnedSeiPayload>();
            sei_parser >> *sei;

            if (sei->type() == SeiType::USER_DATA_REGISTERED) {
              auto rescaled_pts = pts_rescaler.rescale_to_clock(StreamTS(pkt.pts));
              auto rescaled_dts = dts_rescaler.rescale_to_clock(StreamTS(pkt.dts));

              LOG(trace) << "Found CC SEI at pts " << pkt.pts << ", rescaled " << rescaled_pts;

              input.push<SeiKind>(rescaled_pts, rescaled_dts, order, std::move(sei), ctx);

              order++;
            }
          }
        }
      }
    } catch (BinaryParseError &ex) {
      LOG(error) << "Parse error: " << ex;
    }

    return false;
  }
};

class ScteExtractor : public PacketProcessor<ScteKind>
{
private:
  UserDefinedInput &input;
  const ApplicationContext &ctx;

  TSRescaler pts_rescaler;
  TSRescaler dts_rescaler;

public:
  ScteExtractor(StreamTimeBase stream_time_base, UserDefinedInput &input, const ApplicationContext &ctx)
    : input{ input }
    , ctx{ ctx }
    , pts_rescaler{ TSRescaler::clock_relative(ctx.clock, stream_time_base) }
    , dts_rescaler{ TSRescaler::clock_relative(ctx.clock, stream_time_base) }
  {}

  static std::unique_ptr<PacketProcessor<ScteKind>> factory(StreamTimeBase stream_time_base,
                                                            UserDefinedInput &input,
                                                            const ApplicationContext &ctx)
  {
    return std::make_unique<ScteExtractor>(stream_time_base, input, ctx);
  }

  bool process(AVPacket &pkt) override
  {
    try {
      auto parser = Scte35Parser::create(pkt.data, pkt.data + pkt.size);
      while (parser) {
        auto section = std::make_shared<SpliceInfoSection>();
        parser >> *section;

        auto rescaled_pts = pts_rescaler.rescale_to_clock(StreamTS(pkt.pts));
        auto rescaled_dts = dts_rescaler.rescale_to_clock(StreamTS(pkt.dts));

        LOG(trace) << "Found SCTE-35 packet at dts " << pkt.dts << " pts " << pkt.pts << ", rescaled " << rescaled_pts
                   << ": " << *section;

        input.push<ScteKind>(rescaled_pts, rescaled_dts, 0, std::move(section), ctx);
      }
    } catch (BinaryParseError &ex) {
      LOG(error) << "Parse error: " << ex;
    }

    return false;
  }
};
}

static void
post_exit(const InputSpec &input_spec, const ApplicationContext &ctx)
{
  // Remove forward frames from meta queue
  if (auto count = ctx.meta_queue->drop_id(input_spec.id); count > 0) {
    LOG(debug) << "Dropped " << count << " left-over metadata entries from queue";
  } else {
    LOG(debug) << "No left-over metadata on queue";
  }

  // Emit CC Reset SEI packet in very next frame after which the stream was killed,
  // to clear any garbage after current frame.

  // FIXME Beware of race condition between now() and push() calls (output thread might be
  // invoked then), but I think this is rare situation.
  auto reset = build_cc_reset_metadata(input_spec.id, ctx.clock->now() + 1_clock);
  ctx.meta_queue->push(reset);
  LOG(debug) << "Pushed CC Reset at " << reset.pts;
}

void
extractor(std::string input_name, std::shared_ptr<ApplicationContext> ctx)
{
  metamix::log::set_thread_name("input:" + input_name);

  UserDefinedInput &input = *ctx->input_manager->get_input_by_name<UserDefinedInput>(input_name);

  BOOST_SCOPE_EXIT_ALL(&) { post_exit(input.spec(), *ctx); };

  input.is_restart_scheduled(false);

  SinkHandle sink(input.spec().name, input.spec().sink);
  SourceHandle source(input.spec().name, input.spec().source);

  source.open(input.spec().source_format);
  sink.open(input.spec().sink_format);

  sink.init_remuxing(source);

  auto sc = source.classify_streams();

  sc.require<TimeSourceKind>();

  sink.start();

  remux_loop(source,
             sink,
             sc,
             { std::bind(&MaintenanceProcessor::factory, std::ref(input)),
               std::bind(&SeiExtractor::factory, ph::_1, std::ref(input), std::cref(*ctx)),
               std::bind(&ScteExtractor::factory, ph::_1, std::ref(input), std::cref(*ctx)) });
}
}
