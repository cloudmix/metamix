#include "nalu_parser.h"

#include <algorithm>
#include <cassert>

#include <boost/endian/conversion.hpp>
#include <boost/format.hpp>

#include "../binary_parser.h"

#include "nalu.h"

using boost::endian::big_to_native_inplace;

namespace metamix::h264 {

namespace {

uint32_t
parse_avcc_nalu_length(const uint8_t *data, uint32_t nalu_length_size)
{
  assert(0 < nalu_length_size && nalu_length_size <= sizeof(uint32_t));

  union
  {
    uint32_t number;
    uint8_t array[sizeof(uint32_t)];
  } result{ 0 };

  // Copy NALU length to result, right-aligned
  std::copy(data, data + nalu_length_size, result.array + sizeof(result) - nalu_length_size);

  // Convert NALU length to native endian
  big_to_native_inplace(result.number);

  return result.number;
}

std::optional<BinaryParserBounds>
next_avcc_bounds(uint32_t nalu_length_size_minus_one, const uint8_t *startptr, size_t length)
{
  // Check whether there is any data left to parse
  if (length == 0) {
    return std::nullopt;
  }

  uint32_t nalu_length_size = nalu_length_size_minus_one + 1;

  // Check whether there is space for NALU length
  if (nalu_length_size > length) {
    static auto FMT = boost::format("next NALU length size is larger than buffer space available: %1% > %2%");
    throw BinaryParseError((FMT % nalu_length_size % length).str());
  }

  uint32_t nalu_length = parse_avcc_nalu_length(startptr, nalu_length_size);

  // NALU should have some length
  if (nalu_length == 0) {
    throw BinaryParseError("0-sized NALU");
  }

  // NALU length is too large
  if (nalu_length > Nalu::MAX_LENGTH) {
    static auto FMT = boost::format("NALU length is larger than maximum: %1%");
    throw BinaryParseError((FMT % nalu_length).str());
  }

  // Buffer overflow
  if (nalu_length_size + nalu_length > length) {
    static auto FMT = boost::format("next NALU is larger than buffer space available: %1% > %2%");
    throw BinaryParseError((FMT % (nalu_length_size + nalu_length) % length).str());
  }

  // Return NALU boundaries
  return BinaryParserBounds(startptr + nalu_length_size, nalu_length);
}

std::optional<BinaryParserBounds>
next_avcc_bounds_4(const uint8_t *startptr, size_t length)
{
  return next_avcc_bounds(3, startptr, length);
}
}

std::optional<BinaryParserBounds>
nalu_parser_next(const uint8_t *startptr, size_t length)
{
  return next_avcc_bounds_4(startptr, length);
}
}
