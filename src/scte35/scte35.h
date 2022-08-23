#pragma once

#include <cstdint>
#include <cstdlib>
#include <optional>
#include <ostream>
#include <tuple>
#include <variant>
#include <vector>

#include "../clock_types.h"

namespace metamix::scte35 {

struct SpliceTime
{
  static constexpr TS CLOCK_RATE = 90'000;

  std::optional<UTS> pts_time{};

  constexpr bool time_specified() const { return pts_time.has_value(); }

  SpliceTime() = default;
  SpliceTime(std::optional<UTS> pts_time)
    : pts_time(std::move(pts_time))
  {}

  bool operator==(const SpliceTime &rhs) const { return pts_time == rhs.pts_time; }

  bool operator!=(const SpliceTime &rhs) const { return !(rhs == *this); }

  friend std::ostream &operator<<(std::ostream &os, const SpliceTime &spliceTime);
};

struct BreakDuration
{
  bool auto_return;
  UTS duration;

  BreakDuration(bool auto_return, UTS duration)
    : auto_return(auto_return)
    , duration(duration)
  {}

  bool operator==(const BreakDuration &rhs) const
  {
    return std::tie(auto_return, duration) == std::tie(rhs.auto_return, rhs.duration);
  }

  bool operator!=(const BreakDuration &rhs) const { return !(rhs == *this); }

  friend std::ostream &operator<<(std::ostream &os, const BreakDuration &duration);
};

struct SpliceNull
{
  constexpr static uint8_t type = 0x00;

  bool operator==([[maybe_unused]] const SpliceNull &rhs) const { return true; }

  bool operator!=(const SpliceNull &rhs) const { return !(rhs == *this); }

  friend std::ostream &operator<<(std::ostream &os, const SpliceNull &null);
};

struct SpliceSchedule
{
  constexpr static uint8_t type = 0x04;

  struct ProgramSpliceOn
  {
    uint32_t utc_splice_time;

    ProgramSpliceOn(uint32_t utc_splice_time)
      : utc_splice_time(utc_splice_time)
    {}

    bool operator==(const ProgramSpliceOn &rhs) const { return utc_splice_time == rhs.utc_splice_time; }

    bool operator!=(const ProgramSpliceOn &rhs) const { return !(rhs == *this); }

    friend std::ostream &operator<<(std::ostream &os, const ProgramSpliceOn &on);
  };

  struct ProgramSpliceOff
  {
    struct Component
    {
      uint8_t component_tag;
      uint32_t utc_splice_time;

      Component(uint8_t component_tag, uint32_t utc_splice_time)
        : component_tag(component_tag)
        , utc_splice_time(utc_splice_time)
      {}

      bool operator==(const Component &rhs) const
      {
        return std::tie(component_tag, utc_splice_time) == std::tie(rhs.component_tag, rhs.utc_splice_time);
      }

      bool operator!=(const Component &rhs) const { return !(rhs == *this); }

      friend std::ostream &operator<<(std::ostream &os, const Component &component);
    };

    std::vector<Component> components{};

    ProgramSpliceOff(std::vector<Component> components)
      : components(std::move(components))
    {}

    bool operator==(const ProgramSpliceOff &rhs) const { return components == rhs.components; }

    bool operator!=(const ProgramSpliceOff &rhs) const { return !(rhs == *this); }

    friend std::ostream &operator<<(std::ostream &os, const ProgramSpliceOff &off);
  };

  struct CancelOff
  {
    bool out_of_network;
    std::variant<ProgramSpliceOn, ProgramSpliceOff> splice;
    std::optional<BreakDuration> break_duration;
    uint16_t unique_program_id;
    uint8_t avail_num;
    uint8_t avails_expected;

    CancelOff(bool out_of_network,
              std::variant<ProgramSpliceOn, ProgramSpliceOff> splice,
              std::optional<BreakDuration> break_duration,
              uint16_t unique_program_id,
              uint8_t avail_num,
              uint8_t avails_expected)
      : out_of_network(out_of_network)
      , splice(std::move(splice))
      , break_duration(std::move(break_duration))
      , unique_program_id(unique_program_id)
      , avail_num(avail_num)
      , avails_expected(avails_expected)
    {}

    constexpr bool program_splice() const { return std::holds_alternative<ProgramSpliceOn>(splice); }

    constexpr bool duration() const { return break_duration.has_value(); }

    bool operator==(const CancelOff &rhs) const
    {
      return std::tie(out_of_network, splice, break_duration, unique_program_id, avail_num, avails_expected) ==
             std::tie(rhs.out_of_network,
                      rhs.splice,
                      rhs.break_duration,
                      rhs.unique_program_id,
                      rhs.avail_num,
                      rhs.avails_expected);
    }

    bool operator!=(const CancelOff &rhs) const { return !(rhs == *this); }

    friend std::ostream &operator<<(std::ostream &os, const CancelOff &off);
  };

  struct Event
  {
    uint32_t id;
    std::optional<CancelOff> more;

    Event(uint32_t id, std::optional<CancelOff> more)
      : id(id)
      , more(std::move(more))
    {}

    constexpr bool cancel() const { return !more; }

    bool operator==(const Event &rhs) const { return std::tie(id, more) == std::tie(rhs.id, rhs.more); }

    bool operator!=(const Event &rhs) const { return !(rhs == *this); }

    friend std::ostream &operator<<(std::ostream &os, const Event &event);
  };

  std::vector<Event> events{};

  SpliceSchedule(std::vector<Event> events)
    : events(std::move(events))
  {}

  bool operator==(const SpliceSchedule &rhs) const { return events == rhs.events; }

  bool operator!=(const SpliceSchedule &rhs) const { return !(rhs == *this); }

  friend std::ostream &operator<<(std::ostream &os, const SpliceSchedule &schedule);
};

struct SpliceInsert
{
  constexpr static uint8_t type = 0x05;

  struct CancelOff
  {
    struct Component
    {
      uint8_t component_tag;
      std::optional<SpliceTime> splice_time;

      Component(uint8_t component_tag, std::optional<SpliceTime> splice_time)
        : component_tag(component_tag)
        , splice_time(std::move(splice_time))
      {}

      bool operator==(const Component &rhs) const
      {
        return std::tie(component_tag, splice_time) == std::tie(rhs.component_tag, rhs.splice_time);
      }

      bool operator!=(const Component &rhs) const { return !(rhs == *this); }

      friend std::ostream &operator<<(std::ostream &os, const Component &component);
    };

    bool out_of_network;
    bool program_splice;
    bool splice_immediate;
    std::optional<SpliceTime> splice_time;
    std::optional<std::vector<Component>> components;
    std::optional<BreakDuration> break_duration;
    uint16_t unique_program_id;
    uint8_t avail_num;
    uint8_t avails_expected;

    CancelOff(bool out_of_network,
              bool program_splice,
              bool splice_immediate,
              std::optional<SpliceTime> splice_time,
              std::optional<std::vector<Component>> components,
              std::optional<BreakDuration> break_duration,
              uint16_t unique_program_id,
              uint8_t avail_num,
              uint8_t avails_expected)
      : out_of_network(out_of_network)
      , program_splice(program_splice)
      , splice_immediate(splice_immediate)
      , splice_time(std::move(splice_time))
      , components(std::move(components))
      , break_duration(std::move(break_duration))
      , unique_program_id(unique_program_id)
      , avail_num(avail_num)
      , avails_expected(avails_expected)
    {}

    bool duration() const { return break_duration.has_value(); }

    bool operator==(const CancelOff &rhs) const
    {
      return std::tie(out_of_network,
                      program_splice,
                      splice_immediate,
                      splice_time,
                      components,
                      break_duration,
                      unique_program_id,
                      avail_num,
                      avails_expected) == std::tie(rhs.out_of_network,
                                                   rhs.program_splice,
                                                   rhs.splice_immediate,
                                                   rhs.splice_time,
                                                   rhs.components,
                                                   rhs.break_duration,
                                                   rhs.unique_program_id,
                                                   rhs.avail_num,
                                                   rhs.avails_expected);
    }

    bool operator!=(const CancelOff &rhs) const { return !(rhs == *this); }

    friend std::ostream &operator<<(std::ostream &os, const CancelOff &off);
  };

  uint32_t id;
  std::optional<CancelOff> more;

  SpliceInsert(uint32_t id, std::optional<CancelOff> more)
    : id(id)
    , more(std::move(more))
  {}

  constexpr bool cancel() const { return !more; }

  bool operator==(const SpliceInsert &rhs) const { return std::tie(id, more) == std::tie(rhs.id, rhs.more); }

  bool operator!=(const SpliceInsert &rhs) const { return !(rhs == *this); }

  friend std::ostream &operator<<(std::ostream &os, const SpliceInsert &insert);
};

struct TimeSignal
{
  constexpr static uint8_t type = 0x06;

  SpliceTime splice_time;

  TimeSignal(SpliceTime splice_time)
    : splice_time(std::move(splice_time))
  {}

  bool operator==(const TimeSignal &rhs) const { return splice_time == rhs.splice_time; }

  bool operator!=(const TimeSignal &rhs) const { return !(rhs == *this); }

  friend std::ostream &operator<<(std::ostream &os, const TimeSignal &signal);
};

struct BandwidthReservation
{
  constexpr static uint8_t type = 0x07;

  bool operator==([[maybe_unused]] const BandwidthReservation &rhs) const { return true; }

  bool operator!=(const BandwidthReservation &rhs) const { return !(rhs == *this); }

  friend std::ostream &operator<<(std::ostream &os, const BandwidthReservation &reservation);
};

struct PrivateCommand
{
  constexpr static uint8_t type = 0xFF;

  uint32_t identifier;
  std::vector<uint8_t> bytes;

  PrivateCommand(uint32_t identifier, std::vector<uint8_t> bytes)
    : identifier(identifier)
    , bytes(std::move(bytes))
  {}

  bool operator==(const PrivateCommand &rhs) const
  {
    return std::tie(identifier, bytes) == std::tie(rhs.identifier, rhs.bytes);
  }

  bool operator!=(const PrivateCommand &rhs) const { return !(rhs == *this); }

  friend std::ostream &operator<<(std::ostream &os, const PrivateCommand &command);
};

struct AvailDescriptor
{
  constexpr static uint8_t tag = 0x00;
  constexpr static uint8_t length = 8;
  constexpr static uint32_t identifier = 0x43554549;

  uint32_t provider_avail_id;

  AvailDescriptor(uint32_t provider_avail_id)
    : provider_avail_id(provider_avail_id)
  {}

  bool operator==(const AvailDescriptor &rhs) const { return provider_avail_id == rhs.provider_avail_id; }

  bool operator!=(const AvailDescriptor &rhs) const { return !(rhs == *this); }

  friend std::ostream &operator<<(std::ostream &os, const AvailDescriptor &descriptor);
};

struct DtmfDescriptor
{
  constexpr static uint8_t tag = 0x01;
  constexpr static uint32_t identifier = 0x43554549;

  uint8_t preroll;
  std::vector<char> dtmf_chars;

  DtmfDescriptor(uint8_t preroll, std::vector<char> dtmf_chars)
    : preroll(preroll)
    , dtmf_chars(std::move(dtmf_chars))
  {}

  bool operator==(const DtmfDescriptor &rhs) const
  {
    return std::tie(preroll, dtmf_chars) == std::tie(rhs.preroll, rhs.dtmf_chars);
  }

  bool operator!=(const DtmfDescriptor &rhs) const { return !(rhs == *this); }

  friend std::ostream &operator<<(std::ostream &os, const DtmfDescriptor &descriptor);
};

struct SegmentationDescriptor
{
  constexpr static uint8_t tag = 0x02;
  constexpr static uint32_t identifier = 0x43554549;

  std::vector<uint8_t> data; // TODO Implement model for this descriptor.

  SegmentationDescriptor(std::vector<uint8_t> data)
    : data(std::move(data))
  {}

  bool operator==(const SegmentationDescriptor &rhs) const { return data == rhs.data; }

  bool operator!=(const SegmentationDescriptor &rhs) const { return !(rhs == *this); }

  friend std::ostream &operator<<(std::ostream &os, const SegmentationDescriptor &descriptor);
};

struct TimeDescriptor
{
  constexpr static uint8_t tag = 0x03;
  constexpr static uint8_t length = 16;
  constexpr static uint32_t identifier = 0x43554549;

  uint64_t tai_seconds;
  uint32_t tai_nanoseconds;
  uint16_t utc_offset;

  TimeDescriptor(uint64_t tai_seconds, uint32_t tai_nanoseconds, uint16_t utc_offset)
    : tai_seconds(tai_seconds)
    , tai_nanoseconds(tai_nanoseconds)
    , utc_offset(utc_offset)
  {}

  bool operator==(const TimeDescriptor &rhs) const
  {
    return std::tie(tai_seconds, tai_nanoseconds, utc_offset) ==
           std::tie(rhs.tai_seconds, rhs.tai_nanoseconds, rhs.utc_offset);
  }

  bool operator!=(const TimeDescriptor &rhs) const { return !(rhs == *this); }

  friend std::ostream &operator<<(std::ostream &os, const TimeDescriptor &descriptor);
};

struct SpliceInfoSection
{
  using Command =
    std::variant<SpliceNull, SpliceSchedule, SpliceInsert, TimeSignal, BandwidthReservation, PrivateCommand>;

  using Descriptor = std::variant<AvailDescriptor, DtmfDescriptor, SegmentationDescriptor, TimeDescriptor>;
  using Descriptors = std::vector<Descriptor>;

  /// This is an 8-bit field. Its value shall be 0xFC.
  constexpr static uint8_t TABLE_ID = 0xFC;

  /// The section_syntax_indicator is a 1-bit field that should always be set to '0' indicating that MPEG short sections
  /// are to be used.
  constexpr static bool SECTION_SYNTAX_INDICATOR = false;

  /// This is a 1-bit flag that shall be set to 0.
  constexpr static bool PRIVATE_INDICATOR = false;

  // section_length – This is a 12-bit field specifying the number of remaining bytes in the splice_info_section
  // immediately following the section_length field up to the end of the splice_info_section. The value in this field
  // shall not exceed 4093.

  /// Must be zero.
  constexpr static uint8_t PROTOCOL_VERSION = 0;

  /// When this bit is set to '1', it indicates that portions of the splice_info_section, starting with
  /// splice_command_type and ending with and including E_CRC_32, are encrypted. When this bit is set to '0', no part of
  /// this message is encrypted.
  bool encrypted_packet{ false };

  /// This 6 bit unsigned integer specifies which encryption algorithm was used to encrypt the current message. When the
  /// encrypted_packet bit is zero, this field is present but undefined.
  uint8_t encryption_algorithm{ 0 };

  UTS pts_adjustment{ 0 };

  /// An 8 bit unsigned integer that conveys which control word (key) is to be used to decrypt the message. The splicing
  /// device may store up to 256 keys previously provided for this purpose. When the encrypted_packet bit is zero, this
  /// field is present but undefined.
  uint8_t cw_index{ 0 };

  /// A 12-bit value used by the SCTE 35 message provider to assign messages to authorization tiers. This field may take
  /// any value between 0x000 and 0xFFF. The value of 0xFFF provides backwards compatibility and shall be ignored by
  /// downstream equipment. When using tier, the message provider should keep the entire message in a single transport
  /// stream packet.
  uint16_t tier{ 0xfff };

  // splice_command_length – a 12 bit length of the splice command. The length shall represent the number of bytes
  // following the splice_command_type up to but not including the descriptor_loop_length. Devices that are compliant
  // with this version of the standard shall populate this field with the actual length. The value of 0xFFF provides
  // backwards compatibility and shall be ignored by downstream equipment.

  // splice_command_type – An 8-bit unsigned integer which shall be assigned one of the values shown in column labelled
  // splice_command_type value in Table 6.

  Command command{ SpliceNull{} };

  // descriptor_loop_length

  Descriptors descriptors{};

  // alignment_stuffing

  // E_CRC_32

  // CRC_32

  SpliceInfoSection() = default;

  SpliceInfoSection(bool encrypted_packet,
                    uint8_t encryption_algorithm,
                    UTS pts_adjustment,
                    uint8_t cw_index,
                    uint16_t tier,
                    Command command,
                    Descriptors descriptors = Descriptors())
    : encrypted_packet(encrypted_packet)
    , encryption_algorithm(encryption_algorithm)
    , pts_adjustment(pts_adjustment)
    , cw_index(cw_index)
    , tier(tier)
    , command(std::move(command))
    , descriptors(std::move(descriptors))
  {}

  bool operator==(const SpliceInfoSection &rhs) const
  {
    return std::tie(encrypted_packet, encryption_algorithm, pts_adjustment, cw_index, tier, command, descriptors) ==
           std::tie(rhs.encrypted_packet,
                    rhs.encryption_algorithm,
                    rhs.pts_adjustment,
                    rhs.cw_index,
                    rhs.tier,
                    rhs.command,
                    rhs.descriptors);
  }

  bool operator!=(const SpliceInfoSection &rhs) const { return !(rhs == *this); }

  friend std::ostream &operator<<(std::ostream &os, const SpliceInfoSection &section);
};
}
