#include "scte35.h"

#include <cassert>
#include <ostream>

#include "../byte_vector_io.h"
#include "../optional_io.h"
#include "../variant_io.h"

namespace std {

template<class CharType, class CharTrait, typename T>
inline basic_ostream<CharType, CharTrait> &
operator<<(basic_ostream<CharType, CharTrait> &os, const vector<T> &data)
{
  os << "[";
  auto it = std::begin(data);
  if (it != std::end(data)) {
    os << *it++;
    while (it != std::end(data)) {
      os << "," << *it++;
    }
  }
  os << "]";
  return os;
}
}

namespace metamix::scte35 {

std::ostream &
operator<<(std::ostream &os, const SpliceInfoSection &section)
{
  return os << "SpliceInfoSection{"
            << "encrypted_packet=" << section.encrypted_packet << ","
            << "encryption_algorithm=" << static_cast<int>(section.encryption_algorithm) << ","
            << "pts_adjustment=" << section.pts_adjustment << ","
            << "cw_index=" << static_cast<int>(section.cw_index) << ","
            << "tier=" << section.tier << ","
            << "command=" << section.command << ","
            << "descriptors=" << section.descriptors << "}";
}

std::ostream &
operator<<(std::ostream &os, const SpliceTime &spliceTime)
{
  return os << "SpliceTime{"
            << "pts_time=" << spliceTime.pts_time << "}";
}

std::ostream &
operator<<(std::ostream &os, const BreakDuration &duration)
{
  return os << "BreakDuration{"
            << "auto_return=" << duration.auto_return << ","
            << "duration=" << duration.duration << "}";
}

std::ostream &
operator<<(std::ostream &os, [[maybe_unused]] const SpliceNull &null)
{
  return os << "SpliceNull{}";
}

std::ostream &
operator<<(std::ostream &os, const SpliceSchedule::ProgramSpliceOn &on)
{
  return os << "SpliceSchedule::ProgramSpliceOn{"
            << "utc_splice_time=" << on.utc_splice_time << "}";
}

std::ostream &
operator<<(std::ostream &os, const SpliceSchedule::ProgramSpliceOff::Component &component)
{
  return os << "SpliceSchedule::ProgramSpliceOff::Component{"
            << "component_tag=" << static_cast<int>(component.component_tag) << ","
            << "utc_splice_time=" << component.utc_splice_time << "}";
}

std::ostream &
operator<<(std::ostream &os, const SpliceSchedule::ProgramSpliceOff &off)
{
  return os << "SpliceSchedule::ProgramSpliceOff{"
            << "components=" << off.components << "}";
}

std::ostream &
operator<<(std::ostream &os, const SpliceSchedule::CancelOff &off)
{
  return os << "SpliceSchedule::CancelOff{"
            << "out_of_network=" << off.out_of_network << ","
            << "splice=" << off.splice << ","

            << "break_duration=" << off.break_duration << ","
            << "unique_program_id=" << off.unique_program_id << ","

            << "avail_num=" << static_cast<int>(off.avail_num) << ","
            << "avails_expected=" << static_cast<int>(off.avails_expected) << "}";
}

std::ostream &
operator<<(std::ostream &os, const SpliceSchedule::Event &event)
{
  return os << "SpliceSchedule::Event{"
            << "id=" << event.id << ","
            << "more=" << event.more << "}";
}

std::ostream &
operator<<(std::ostream &os, const SpliceSchedule &schedule)
{
  return os << "SpliceSchedule{"
            << "events=" << schedule.events << "}";
}

std::ostream &
operator<<(std::ostream &os, const SpliceInsert::CancelOff::Component &component)
{
  return os << "SpliceInsert::CancelOff::Component{"
            << "component_tag=" << static_cast<int>(component.component_tag) << ","
            << "splice_time=" << component.splice_time << "}";
}

std::ostream &
operator<<(std::ostream &os, const SpliceInsert::CancelOff &off)
{
  return os << "SpliceInsert::CancelOff{"
            << "out_of_network=" << off.out_of_network << ","
            << "program_splice=" << off.program_splice << ","

            << "splice_immediate=" << off.splice_immediate << ","
            << "splice_time=" << off.splice_time << ","

            << "components=" << off.components << ","
            << "break_duration=" << off.break_duration << ","

            << "unique_program_id=" << off.unique_program_id << ","
            << "avail_num=" << static_cast<int>(off.avail_num) << ","

            << "avails_expected=" << static_cast<int>(off.avails_expected) << "}";
}

std::ostream &
operator<<(std::ostream &os, const SpliceInsert &insert)
{
  return os << "SpliceInsert{"
            << "id=" << insert.id << ","
            << "more=" << insert.more << "}";
}

std::ostream &
operator<<(std::ostream &os, const TimeSignal &signal)
{
  return os << "TimeSignal{"
            << "splice_time=" << signal.splice_time << "}";
}

std::ostream &
operator<<(std::ostream &os, [[maybe_unused]] const BandwidthReservation &reservation)
{
  return os << "BandwidthReservation{}";
}

std::ostream &
operator<<(std::ostream &os, const PrivateCommand &command)
{
  return os << "PrivateCommand{"
            << "identifier=" << command.identifier << ","
            << "bytes=" << command.bytes << "}";
}

std::ostream &
operator<<(std::ostream &os, const AvailDescriptor &descriptor)
{
  return os << "AvailDescriptor{"
            << "provider_avail_id=" << descriptor.provider_avail_id << "}";
}

std::ostream &
operator<<(std::ostream &os, const DtmfDescriptor &descriptor)
{
  return os << "DtmfDescriptor{"
            << "preroll=" << static_cast<int>(descriptor.preroll) << ","
            << "dtmf_chars=" << descriptor.dtmf_chars << "}";
}

std::ostream &
operator<<(std::ostream &os, const SegmentationDescriptor &descriptor)
{
  return os << "SegmentationDescriptor{"
            << "data=" << descriptor.data << "}";
}

std::ostream &
operator<<(std::ostream &os, const TimeDescriptor &descriptor)
{
  return os << "TimeDescriptor{"
            << "tai_seconds=" << descriptor.tai_seconds << ","
            << "tai_nanoseconds=" << descriptor.tai_nanoseconds << ","
            << "utc_offset=" << descriptor.utc_offset << "}";
}
}
