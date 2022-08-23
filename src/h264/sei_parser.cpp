#include "sei_parser.h"

namespace metamix::h264 {

namespace {

unsigned int
parse_variadic_length_int(const uint8_t *endptr, const uint8_t *&startptr)
{
  unsigned int x = 0;
  while (*startptr == 0xFF) {
    x += 255;
    startptr++;
    if (startptr == endptr) {
      throw BinaryParseError("malformed SEI");
    }
  }
  x += *startptr;
  startptr++;
  return x;
}
}

std::optional<SeiParserBounds>
sei_parser_next(const uint8_t *startptr, size_t length)
{
  if (length == 0) {
    return std::nullopt;
  }

  const uint8_t *endptr = startptr + length;

  unsigned int payload_type = parse_variadic_length_int(endptr, startptr);
  size_t payload_size = parse_variadic_length_int(endptr, startptr);

  if (payload_size > static_cast<unsigned int>(endptr - startptr)) {
    throw BinaryParseError("malformed SEI");
  }

  return SeiParserBounds(startptr, payload_size, payload_type);
}
}
