#include "parser.h"

#include <cassert>
#include <string>
#include <type_traits>

#include <boost/endian/conversion.hpp>
#include <boost/format.hpp>

#include "../binary_parser_util.h"

#include "crc32.h"

using boost::endian::big_to_native;
using boost::endian::native_to_big;

namespace metamix::scte35 {

namespace {

/// Minimal length of slice info section in bytes, counts in all fields up to splice_command_type,
/// descriptor_loop_length and CRC_32.
constexpr int MIN_SIS_LENGTH = 1 + 2 + 1 + 1 + 4 + 1 + 3 + 1 + 2 + 4;

#define TEST_CONST(LHS, OP, RHS)                                                                                       \
  if (!((LHS)OP(RHS))) {                                                                                               \
    constexpr auto f =                                                                                                 \
      "condition '" BOOST_STRINGIZE(LHS OP RHS) "' failed, " BOOST_STRINGIZE(LHS) "=%1% " BOOST_STRINGIZE(RHS) "=%2%"; \
    throw BinaryParseError((boost::format(f) % (LHS) % (RHS)).str());                                                  \
  }

#define TEST_CONST_EQ(VAR, EXPECTED) TEST_CONST(VAR, ==, EXPECTED)

SpliceTime
parse_splice_time(const uint8_t *&ptr, const uint8_t *endptr)
{
  bool time_specified_flag = scan_flag(ptr, endptr, 0b1000'0000);
  if (time_specified_flag) {
    return SpliceTime(read_33(ptr, endptr));
  } else {
    read_8(ptr, endptr);
    return SpliceTime(std::nullopt);
  }
}

BreakDuration
parse_break_duration(const uint8_t *&ptr, const uint8_t *endptr)
{
  bool auto_return = scan_flag(ptr, endptr, 0b1000'0000);
  uint64_t duration = read_33(ptr, endptr);
  return BreakDuration(auto_return, duration);
}

std::variant<SpliceSchedule::ProgramSpliceOn, SpliceSchedule::ProgramSpliceOff>
parse_splice_schedule_event_program_splice(const uint8_t *&ptr, const uint8_t *endptr, bool program_splice_flag)
{
  if (program_splice_flag) {
    auto utc_splice_time = read_32(ptr, endptr);
    return SpliceSchedule::ProgramSpliceOn(utc_splice_time);
  } else {
    auto component_count = read_8(ptr, endptr);
    TEST_CONST(component_count, >=, 1);
    std::vector<SpliceSchedule::ProgramSpliceOff::Component> components;
    for (auto i = 0; i < component_count; i++) {
      auto component_tag = read_8(ptr, endptr);
      auto utc_splice_time = read_32(ptr, endptr);
      components.emplace_back(component_tag, utc_splice_time);
    }
    return SpliceSchedule::ProgramSpliceOff(std::move(components));
  }
}

SpliceSchedule::Event
parse_splice_schedule_event(const uint8_t *&ptr, const uint8_t *endptr)
{
  auto splice_event_id = read_32(ptr, endptr);
  bool splice_event_cancel_indicator = scan_flag(ptr, endptr, 0b1000'0000);
  read_8(ptr, endptr);

  if (splice_event_cancel_indicator) {
    return SpliceSchedule::Event(splice_event_id, std::nullopt);
  }

  bool out_of_network_indicator = scan_flag(ptr, endptr, 0b1000'0000);
  bool program_splice_flag = scan_flag(ptr, endptr, 0b0100'0000);
  bool duration_flag = scan_flag(ptr, endptr, 0b0010'0000);
  read_8(ptr, endptr);

  auto program_splice = parse_splice_schedule_event_program_splice(ptr, endptr, program_splice_flag);

  std::optional<BreakDuration> break_duration;
  if (duration_flag) {
    break_duration = parse_break_duration(ptr, endptr);
  }

  auto unique_program_id = read_16(ptr, endptr);
  auto avail_num = read_8(ptr, endptr);
  auto avails_expected = read_8(ptr, endptr);

  return SpliceSchedule::Event(
    splice_event_id,
    SpliceSchedule::CancelOff(
      out_of_network_indicator, program_splice, break_duration, unique_program_id, avail_num, avails_expected));
}

SpliceSchedule
parse_splice_schedule(const uint8_t *&ptr, const uint8_t *endptr, [[maybe_unused]] uint16_t splice_command_length)
{
  std::vector<SpliceSchedule::Event> events;
  auto splice_count = read_8(ptr, endptr);
  for (auto i = 0; i < splice_count; i++) {
    events.push_back(parse_splice_schedule_event(ptr, endptr));
  }
  return SpliceSchedule(std::move(events));
}

SpliceInsert
parse_splice_insert(const uint8_t *&ptr, const uint8_t *endptr, [[maybe_unused]] uint16_t splice_command_length)
{
  auto splice_event_id = read_32(ptr, endptr);
  bool splice_event_cancel_indicator = scan_flag(ptr, endptr, 0b1000'0000);
  read_8(ptr, endptr);

  if (splice_event_cancel_indicator) {
    return SpliceInsert(splice_event_id, std::nullopt);
  }

  bool out_of_network_indicator = scan_flag(ptr, endptr, 0b1000'0000);
  bool program_splice_flag = scan_flag(ptr, endptr, 0b0100'0000);
  bool duration_flag = scan_flag(ptr, endptr, 0b0010'0000);
  bool splice_immediate_flag = scan_flag(ptr, endptr, 0b0001'0000);
  read_8(ptr, endptr);

  std::optional<SpliceTime> splice_time;
  if (program_splice_flag && !splice_immediate_flag) {
    splice_time = parse_splice_time(ptr, endptr);
  }

  std::optional<std::vector<SpliceInsert::CancelOff::Component>> components;
  if (!program_splice_flag) {
    auto component_count = read_8(ptr, endptr);
    TEST_CONST(component_count, >=, 1);
    std::vector<SpliceInsert::CancelOff::Component> components_v;
    for (auto i = 0; i < component_count; i++) {
      auto component_tag = read_8(ptr, endptr);
      std::optional<SpliceTime> component_splice_time;
      if (!splice_immediate_flag) {
        component_splice_time = parse_splice_time(ptr, endptr);
      }
      components_v.emplace_back(component_tag, component_splice_time);
    }
    components = std::move(components_v);
  }

  std::optional<BreakDuration> break_duration;
  if (duration_flag) {
    break_duration = parse_break_duration(ptr, endptr);
  }

  auto unique_program_id = read_16(ptr, endptr);
  auto avail_num = read_8(ptr, endptr);
  auto avails_expected = read_8(ptr, endptr);

  return SpliceInsert(splice_event_id,
                      SpliceInsert::CancelOff(out_of_network_indicator,
                                              program_splice_flag,
                                              splice_immediate_flag,
                                              splice_time,
                                              std::move(components),
                                              break_duration,
                                              unique_program_id,
                                              avail_num,
                                              avails_expected));
}

TimeSignal
parse_time_signal(const uint8_t *&ptr, const uint8_t *endptr, [[maybe_unused]] uint16_t splice_command_length)
{
  return TimeSignal(parse_splice_time(ptr, endptr));
}

PrivateCommand
parse_private_command(const uint8_t *&ptr, const uint8_t *endptr, uint16_t splice_command_length)
{
  TEST_CONST(splice_command_length, >, 4);
  TEST_CONST(splice_command_length, <, 0xfff);
  uint32_t identifier = read_32(ptr, endptr);
  std::vector<uint8_t> bytes(ptr + 4, ptr + splice_command_length);
  ptr += splice_command_length;
  return PrivateCommand(identifier, std::move(bytes));
}

AvailDescriptor
parse_avail_descriptor(const uint8_t *&ptr,
                       const uint8_t *endptr,
                       uint8_t splice_descriptor_tag,
                       uint8_t descriptor_length,
                       uint32_t identifier)
{
  TEST_CONST_EQ(splice_descriptor_tag, AvailDescriptor::tag);
  TEST_CONST_EQ(descriptor_length, AvailDescriptor::length);
  TEST_CONST_EQ(identifier, AvailDescriptor::identifier);

  return AvailDescriptor(read_32(ptr, endptr));
}

DtmfDescriptor
parse_dtmf_descriptor(const uint8_t *&ptr,
                      const uint8_t *endptr,
                      uint8_t splice_descriptor_tag,
                      uint8_t descriptor_length,
                      uint32_t identifier)
{
  TEST_CONST_EQ(splice_descriptor_tag, DtmfDescriptor::tag);
  TEST_CONST_EQ(identifier, DtmfDescriptor::identifier);

  auto preroll = read_8(ptr, endptr);

  auto dtmf_count = scan_8(ptr, endptr, 0b1110'0000) >> 5;
  read_8(ptr, endptr);

  TEST_CONST_EQ(descriptor_length, 4 + 1 + dtmf_count);

  {
    auto available_length = endptr - ptr;
    TEST_CONST(dtmf_count, <=, available_length);
  }

  std::vector<char> dtmf_chars(ptr, ptr + dtmf_count);

  ptr += dtmf_count;

  return DtmfDescriptor(preroll, std::move(dtmf_chars));
}

SegmentationDescriptor
parse_segmentation_descriptor(const uint8_t *&ptr,
                              const uint8_t *real_endptr,
                              uint8_t splice_descriptor_tag,
                              uint8_t descriptor_length,
                              uint32_t identifier)
{
  TEST_CONST_EQ(splice_descriptor_tag, SegmentationDescriptor::tag);
  TEST_CONST_EQ(identifier, SegmentationDescriptor::identifier);

  auto endptr = ptr + descriptor_length - 4;
  TEST_CONST(endptr, <=, real_endptr);
  auto data = std::vector<uint8_t>(ptr, endptr);
  ptr = endptr;
  return SegmentationDescriptor(std::move(data));
}

TimeDescriptor
parse_time_descriptor(const uint8_t *&ptr,
                      const uint8_t *endptr,
                      uint8_t splice_descriptor_tag,
                      uint8_t descriptor_length,
                      uint32_t identifier)
{
  TEST_CONST_EQ(splice_descriptor_tag, TimeDescriptor::tag);
  TEST_CONST_EQ(descriptor_length, TimeDescriptor::length);
  TEST_CONST_EQ(identifier, TimeDescriptor::identifier);

  return TimeDescriptor(read_48(ptr, endptr), read_32(ptr, endptr), read_16(ptr, endptr));
}

template<typename OutputIt>
void
parse_descriptors(const uint8_t *&ptr, const uint8_t *endptr, const uint8_t *real_endptr, OutputIt out)
{
  TEST_CONST(endptr, <, real_endptr);

  while (ptr < endptr) {
    auto splice_descriptor_tag = read_8(ptr, endptr);

    auto descriptor_length = read_8(ptr, endptr);

    // Verify descriptor_length
    {
      auto available_length = endptr - ptr;
      TEST_CONST(descriptor_length, <=, available_length);
    }

    auto identifier = read_32(ptr, endptr);

    switch (splice_descriptor_tag) {
    case AvailDescriptor::tag:
      *out++ = parse_avail_descriptor(ptr, endptr, splice_descriptor_tag, descriptor_length, identifier);
      break;
    case DtmfDescriptor::tag:
      *out++ = parse_dtmf_descriptor(ptr, endptr, splice_descriptor_tag, descriptor_length, identifier);
      break;
    case SegmentationDescriptor::tag:
      *out++ = parse_segmentation_descriptor(ptr, endptr, splice_descriptor_tag, descriptor_length, identifier);
      break;
    case TimeDescriptor::tag:
      *out++ = parse_time_descriptor(ptr, endptr, splice_descriptor_tag, descriptor_length, identifier);
      break;
    default:
      throw BinaryParseError("unknown splice descriptor tag: " + std::to_string(splice_descriptor_tag));
    }
  }
}

uint16_t
read_splice_info_section_until_length(const uint8_t *&ptr, const uint8_t *endptr, bool &chopped_prefix)
{
  assert(ptr <= endptr);
  TEST_CONST(endptr - ptr, >=, MIN_SIS_LENGTH);

  const auto startptr = ptr;

  // Skip pointer_field == 0x00 if present
  if (scan_8(ptr, endptr) == 0x00) {
    read_8(ptr, endptr);
    chopped_prefix = true;
  } else {
    chopped_prefix = false;
  }

  // Read table_id and check if it is equal to 0xFC
  uint8_t table_id = read_8(ptr, endptr);
  TEST_CONST_EQ(table_id, SpliceInfoSection::TABLE_ID);

  // Read and check section_syntax_indicator, private_indicator and section_length
  bool section_syntax_indicator = scan_flag(ptr, endptr, 0b1000'0000);
  TEST_CONST_EQ(section_syntax_indicator, SpliceInfoSection::SECTION_SYNTAX_INDICATOR);

  bool private_indicator = scan_flag(ptr, endptr, 0b0100'0000);
  TEST_CONST_EQ(private_indicator, SpliceInfoSection::PRIVATE_INDICATOR);

  uint16_t section_length = read_12_low(ptr, endptr);

  auto header_length = static_cast<uint16_t>(ptr - startptr);

  assert(header_length == 3 || header_length == 4);

  TEST_CONST(section_length, >=, MIN_SIS_LENGTH - header_length);
  TEST_CONST(section_length, <=, 4093);

  {
    size_t available_length = endptr - ptr;
    TEST_CONST(section_length, <=, available_length);
  }

  return section_length;
}
}

std::optional<BinaryParserBounds>
scte35_parser_next(const uint8_t *startptr, size_t length)
{
  if (length < MIN_SIS_LENGTH) {
    return std::nullopt;
  }

  auto ptr = startptr;
  const auto endptr = startptr + length;

  bool chopped_prefix;
  auto section_length = read_splice_info_section_until_length(ptr, endptr, chopped_prefix);

  if (chopped_prefix) {
    startptr++;
  }

  auto header_length = static_cast<uint16_t>(ptr - startptr);

  return BinaryParserBounds(startptr, header_length + section_length);
}

SpliceInfoSection scte35_parser_pack([[maybe_unused]] const BinaryParserContext &ctx, const BinaryParserBounds &bounds)
{
  static auto UNSUPPORTED_TYPE = boost::format("unsupported splice command type: 0x%1$x");
  static auto CMDLEN_DIFF = boost::format("consumed command length: %1% differs from splice_command_length: %2%");

  assert(bounds.startptr() >= ctx.startptr());
  assert(bounds.length() >= MIN_SIS_LENGTH);

  SpliceInfoSection s{};

  auto startptr = bounds.startptr();
  auto ptr = bounds.startptr();
  const auto endptr = bounds.startptr() + bounds.length();

  bool chopped_prefix;
  [[maybe_unused]] uint16_t section_length = read_splice_info_section_until_length(ptr, endptr, chopped_prefix);
  assert(section_length == endptr - ptr);

  if (chopped_prefix) {
    startptr++;
  }

  // Read and check protocol version
  uint8_t protocol_version = read_8(ptr, endptr);
  TEST_CONST_EQ(protocol_version, SpliceInfoSection::PROTOCOL_VERSION);

  // Read encryption data
  s.encrypted_packet = scan_flag(ptr, endptr, 0b1000'0000);
  s.encryption_algorithm = scan_8(ptr, endptr, 0b0111'1110) >> 1;

  if (s.encrypted_packet) {
    throw BinaryParseError("encrypted SCTE-35 packets are not supported");
  }

  // Read pts adjustment
  s.pts_adjustment = read_33(ptr, endptr);
  assert(s.pts_adjustment <= (1ULL << 33));

  // Read cw index
  s.cw_index = read_8(ptr, endptr);

  // Read tier
  s.tier = scan_12_high(ptr, endptr);
  assert(s.tier <= 0xfff);
  read_8(ptr, endptr);

  // Read command lengths & type
  uint16_t splice_command_length = read_12_low(ptr, endptr);
  assert(splice_command_length <= 0xfff);

  uint8_t splice_command_type = read_8(ptr, endptr);

  // Save command start pointer
  const auto command_start_ptr = ptr;

  // Test splice_command_length is not too long
  if (splice_command_length != 0xfff) {
    TEST_CONST(splice_command_length, <, endptr - ptr);
  }

  // Parse command
  switch (splice_command_type) {
  case SpliceNull::type:
    s.command = SpliceNull{};
    break;
  case SpliceSchedule::type:
    s.command = parse_splice_schedule(ptr, endptr, splice_command_length);
    break;
  case SpliceInsert::type:
    s.command = parse_splice_insert(ptr, endptr, splice_command_length);
    break;
  case TimeSignal::type:
    s.command = parse_time_signal(ptr, endptr, splice_command_length);
    break;
  case BandwidthReservation::type:
    s.command = BandwidthReservation{};
    break;
  case PrivateCommand::type:
    s.command = parse_private_command(ptr, endptr, splice_command_length);
    break;
  default:
    throw BinaryParseError((UNSUPPORTED_TYPE % static_cast<int>(splice_command_type)).str());
  }

  const auto command_end_ptr = ptr;

  // Validate splice_command_length
  if (splice_command_length != 0xfff) {
    auto len = static_cast<uint16_t>(command_end_ptr - command_start_ptr);
    if (len != splice_command_length) {
      throw BinaryParseError((CMDLEN_DIFF % len % splice_command_length).str());
    }
  }

  // Read descriptor_loop_length
  uint16_t descriptor_loop_length = read_16(ptr, endptr);

  if (descriptor_loop_length > 0) {
    // Test descriptor_loop_length is not too long
    {
      auto available_length = endptr - ptr;
      TEST_CONST(descriptor_loop_length, <, available_length);
    }

    // Parse descriptors
    parse_descriptors(ptr, ptr + descriptor_loop_length, endptr, std::back_inserter(s.descriptors));
  }

  // Skip encrypted CRC
  if (s.encrypted_packet) {
    read_32(ptr, endptr);
  }

  // Read and validate CRC
  read_32(ptr, endptr);
  auto crc32 = CRC32::compute(startptr, endptr);
  if (crc32 != 0) {
    throw BinaryParseError("CRC32 checksum did not match");
  }

  // Check we reached end of packet
  assert(ptr == endptr);

  return s;
}
}
