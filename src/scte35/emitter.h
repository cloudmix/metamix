#pragma once

#include "../binary_emitter_util.h"

#include "crc32.h"
#include "scte35.h"

namespace metamix::scte35 {

inline constexpr size_t
emit_size_hint(const SpliceTime &splice_time)
{
  return splice_time.pts_time.has_value() ? 5 : 1;
}

inline constexpr size_t
emit_size_hint(const BreakDuration &)
{
  return 5;
}

inline constexpr size_t
emit_size_hint(const SpliceNull &)
{
  return 0;
}

inline constexpr size_t
emit_size_hint(const SpliceSchedule::ProgramSpliceOn &)
{
  return 4;
}

inline size_t
emit_size_hint(const SpliceSchedule::ProgramSpliceOff &splice)
{
  return splice.components.size() * 5;
}

inline size_t
emit_size_hint(const SpliceSchedule::CancelOff &more)
{
  size_t sum = 1 + 4;
  std::visit([](const auto &x) { return emit_size_hint(x); }, more.splice);
  if (more.break_duration)
    sum += emit_size_hint(*more.break_duration);
  return sum;
}

inline size_t
emit_size_hint(const SpliceSchedule::Event &event)
{
  return 5 + (event.more.has_value() ? emit_size_hint(*event.more) : 0);
}

inline size_t
emit_size_hint(const SpliceSchedule &command)
{
  size_t sum = 1;
  for (const auto &event : command.events) {
    sum += emit_size_hint(event);
  }
  return sum;
}

inline size_t
emit_size_hint(const SpliceInsert::CancelOff::Component &component)
{
  return 1 + (component.splice_time.has_value() ? emit_size_hint(*component.splice_time) : 0);
}

inline size_t
emit_size_hint(const std::vector<SpliceInsert::CancelOff::Component> &components)
{
  size_t sum = 1;
  for (const auto &component : components) {
    sum += emit_size_hint(component);
  }
  return sum;
}

inline size_t
emit_size_hint(const SpliceInsert::CancelOff &more)
{
  size_t sum = 1 + 4;
  if (more.splice_time)
    sum += emit_size_hint(*more.splice_time);
  if (more.components)
    sum += emit_size_hint(*more.components);
  if (more.break_duration)
    sum += emit_size_hint(*more.break_duration);
  return sum;
}

inline size_t
emit_size_hint(const SpliceInsert &command)
{
  return 5 + (command.more.has_value() ? emit_size_hint(*command.more) : 0);
}

inline constexpr size_t
emit_size_hint(const TimeSignal &command)
{
  return emit_size_hint(command.splice_time);
}

inline constexpr size_t
emit_size_hint(const BandwidthReservation &)
{
  return 0;
}

inline size_t
emit_size_hint(const PrivateCommand &command)
{
  return 4 + (command.bytes.size() * sizeof(decltype(command.bytes)::value_type));
}

inline size_t
emit_size_hint(const SpliceInfoSection::Command &command)
{
  return std::visit([](const auto &x) { return emit_size_hint(x); }, command);
}

inline constexpr size_t
emit_size_hint(const AvailDescriptor &descriptor)
{
  return 2 + descriptor.length;
}

inline size_t
emit_size_hint(const DtmfDescriptor &descriptor)
{
  return 8 + descriptor.dtmf_chars.size();
}

inline size_t
emit_size_hint(const SegmentationDescriptor &descriptor)
{
  return 6 + descriptor.data.size();
}

inline constexpr size_t
emit_size_hint(const TimeDescriptor &descriptor)
{
  return 2 + descriptor.length;
}

inline size_t
emit_size_hint(const SpliceInfoSection::Descriptor &descriptor)
{
  return std::visit([](const auto &x) { return emit_size_hint(x); }, descriptor);
}

inline size_t
emit_size_hint(const SpliceInfoSection::Descriptors &descriptors)
{
  size_t sum = 0;
  for (const auto &descriptor : descriptors) {
    sum += emit_size_hint(descriptor);
  }
  return sum;
}

namespace detail {

inline size_t
compute_section_length(const SpliceInfoSection &section)
{
  return 17 + emit_size_hint(section.command) + emit_size_hint(section.descriptors);
}
}

inline size_t
emit_size_hint(const SpliceInfoSection &section)
{
  return 3 + detail::compute_section_length(section);
}

template<class OutputIt>
OutputIt
emit(const SpliceTime &splice_time, OutputIt out)
{
  if (splice_time.pts_time) {
    out = write_33_prefix(0xff, *splice_time.pts_time, out);
  } else {
    out = write_8(0x7f, out);
  }
  return out;
}

template<class OutputIt>
OutputIt
emit(const BreakDuration &break_duration, OutputIt out)
{
  out = write_33_prefix(static_cast<uint8_t>(break_duration.auto_return) << 7, break_duration.duration, out);
  return out;
}

template<class OutputIt>
OutputIt
emit(const SpliceNull &, OutputIt out)
{
  return out;
}

template<class OutputIt>
OutputIt
emit(const SpliceSchedule::ProgramSpliceOn &splice, OutputIt out)
{
  out = write_32(splice.utc_splice_time, out);
  return out;
}

template<class OutputIt>
OutputIt
emit(const SpliceSchedule::ProgramSpliceOff::Component &component, OutputIt out)
{
  out = write_8(component.component_tag, out);
  out = write_32(component.utc_splice_time, out);
  return out;
}

template<class OutputIt>
OutputIt
emit(const SpliceSchedule::ProgramSpliceOff &splice, OutputIt out)
{
  out = write_8(splice.components.size(), out);
  for (const auto &component : splice.components) {
    out = emit(component, out);
  }
  return out;
}

template<class OutputIt>
OutputIt
emit(const SpliceSchedule::CancelOff &more, OutputIt out)
{
  auto out_of_network_indicator = more.out_of_network;
  auto program_splice_flag = more.program_splice();
  auto duration_flag = more.duration();
  out = write_8((out_of_network_indicator << 7) | (program_splice_flag << 6) | (duration_flag << 5) | 0b0001'1111, out);
  std::visit([&out](const auto &x) { out = emit(x, out); }, more.splice);
  if (more.break_duration)
    out = emit(*more.break_duration, out);
  out = write_16(more.unique_program_id, out);
  out = write_8(more.avail_num, out);
  out = write_8(more.avails_expected, out);
  return out;
}

template<class OutputIt>
OutputIt
emit(const SpliceSchedule::Event &event, OutputIt out)
{
  out = write_32(event.id, out);
  if (event.more) {
    out = write_8(0x7f, out);
    out = emit(*event.more, out);
  } else {
    out = write_8(0xff, out);
  }
  return out;
}

template<class OutputIt>
OutputIt
emit(const SpliceSchedule &command, OutputIt out)
{
  out = write_8(command.events.size(), out);
  for (const auto &event : command.events) {
    out = emit(event, out);
  }
  return out;
}

template<class OutputIt>
OutputIt
emit(const SpliceInsert::CancelOff::Component &component, OutputIt out)
{
  out = write_8(component.component_tag, out);
  if (component.splice_time)
    out = emit(*component.splice_time, out);
  return out;
}

template<class OutputIt>
OutputIt
emit(const std::vector<SpliceInsert::CancelOff::Component> &components, OutputIt out)
{
  out = write_8(components.size(), out);
  for (const auto &component : components) {
    out = emit(component, out);
  }
  return out;
}

template<class OutputIt>
OutputIt
emit(const SpliceInsert::CancelOff &more, OutputIt out)
{
  auto out_of_network_indicator = more.out_of_network;
  auto program_splice_flag = more.program_splice;
  auto duration_flag = more.duration();
  auto splice_immediate_flag = more.splice_immediate;
  out = write_8((out_of_network_indicator << 7) | (program_splice_flag << 6) | (duration_flag << 5) |
                  (splice_immediate_flag << 4) | 0b0000'1111,
                out);
  if (more.splice_time)
    out = emit(*more.splice_time, out);
  if (more.components)
    out = emit(*more.components, out);
  if (more.break_duration)
    out = emit(*more.break_duration, out);
  out = write_16(more.unique_program_id, out);
  out = write_8(more.avail_num, out);
  out = write_8(more.avails_expected, out);
  return out;
}

template<class OutputIt>
OutputIt
emit(const SpliceInsert &command, OutputIt out)
{
  out = write_32(command.id, out);
  if (command.more) {
    out = write_8(0x7f, out);
    out = emit(*command.more, out);
  } else {
    out = write_8(0xff, out);
  }
  return out;
}

template<class OutputIt>
OutputIt
emit(const TimeSignal &command, OutputIt out)
{
  out = emit(command.splice_time, out);
  return out;
}

template<class OutputIt>
OutputIt
emit(const BandwidthReservation &, OutputIt out)
{
  return out;
}

template<class OutputIt>
OutputIt
emit(const PrivateCommand &command, OutputIt out)
{
  out = write_32(command.identifier, out);
  for (uint8_t byte : command.bytes) {
    out = write_8(byte, out);
  }
  return out;
}

template<class OutputIt>
OutputIt
emit(const AvailDescriptor &descriptor, OutputIt out)
{
  out = write_8(descriptor.tag, out);
  out = write_8(descriptor.length, out);
  out = write_32(descriptor.identifier, out);
  out = write_32(descriptor.provider_avail_id, out);
  return out;
}

template<class OutputIt>
OutputIt
emit(const DtmfDescriptor &descriptor, OutputIt out)
{
  out = write_8(descriptor.tag, out);
  out = write_8(emit_size_hint(descriptor) - 2, out);
  out = write_32(descriptor.identifier, out);
  out = write_8(descriptor.preroll, out);
  out = write_8(descriptor.dtmf_chars.size() << 5, out);
  for (uint8_t chr : descriptor.dtmf_chars) {
    out = write_8(chr, out);
  }
  return out;
}

template<class OutputIt>
OutputIt
emit(const SegmentationDescriptor &descriptor, OutputIt out)
{
  out = write_8(descriptor.tag, out);
  out = write_8(emit_size_hint(descriptor) - 2, out);
  out = write_32(descriptor.identifier, out);
  for (uint8_t byte : descriptor.data) {
    out = write_8(byte, out);
  }
  return out;
}

template<class OutputIt>
OutputIt
emit(const TimeDescriptor &descriptor, OutputIt out)
{
  out = write_8(descriptor.tag, out);
  out = write_8(descriptor.length, out);
  out = write_32(descriptor.identifier, out);
  out = write_48(descriptor.tai_seconds, out);
  out = write_32(descriptor.tai_nanoseconds, out);
  out = write_16(descriptor.utc_offset, out);
  return out;
}

template<class OutputIt>
OutputIt
emit(const SpliceInfoSection &section, OutputIt real_out)
{
  CRC32 crc;
  auto out = make_crc32_output_iterator(real_out, crc);

  out = write_8(section.TABLE_ID, out);
  out = write_12_prefix((section.SECTION_SYNTAX_INDICATOR << 3) | (section.PRIVATE_INDICATOR << 2) | 0b0011,
                        detail::compute_section_length(section),
                        out);
  out = write_8(section.PROTOCOL_VERSION, out);
  out = write_33_prefix((static_cast<uint8_t>(section.encrypted_packet) << 7) |
                          ((section.encryption_algorithm << 1) & 0b0111'1110),
                        section.pts_adjustment,
                        out);
  out = write_8(section.cw_index, out);
  out = write_12_pair(section.tier, static_cast<uint16_t>(emit_size_hint(section.command)), out);
  std::visit(
    [&out](const auto &cmd) {
      out = write_8(cmd.type, out);
      out = emit(cmd, out);
    },
    section.command);
  out = write_16(emit_size_hint(section.descriptors), out);
  for (const auto &descriptor : section.descriptors) {
    std::visit([&out](const auto &desc) { out = emit(desc, out); }, descriptor);
  }
  out = write_32(crc, out);

  return real_out;
}
}
