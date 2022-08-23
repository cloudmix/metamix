#pragma once

#include <cassert>
#include <cstdint>
#include <string>

#include "binary_parser.h"

namespace metamix {

inline void
require_distance(const uint8_t *a, const uint8_t *b, int distance)
{
  assert(b >= a);
  if (b - a < distance) {
    throw BinaryParseError("expected at least " + std::to_string(distance) + " bytes, but got only " +
                           std::to_string(static_cast<int>(b - a)));
  }
}

inline uint8_t
scan_8(const uint8_t *ptr, const uint8_t *endptr, uint8_t mask = 0xFF)
{
  require_distance(ptr, endptr, 1);
  return ptr[0] & mask;
}

inline uint8_t
read_8(const uint8_t *&ptr, const uint8_t *endptr, uint8_t mask = 0xFF)
{
  auto val = scan_8(ptr, endptr, mask);
  ptr++;
  return val;
}

inline bool
scan_flag(const uint8_t *ptr, const uint8_t *endptr, uint8_t mask)
{
  return static_cast<bool>(scan_8(ptr, endptr, mask));
}

inline bool
read_flag(const uint8_t *ptr, const uint8_t *endptr, uint8_t mask)
{
  return static_cast<bool>(read_8(ptr, endptr, mask));
}

inline uint16_t
scan_12_high(const uint8_t *ptr, const uint8_t *endptr)
{
  require_distance(ptr, endptr, 2);
  return (static_cast<uint16_t>(ptr[0]) << 4) + ((ptr[1] & 0b1111'0000) >> 4);
}

inline uint16_t
read_12_high(const uint8_t *&ptr, const uint8_t *endptr)
{
  auto val = scan_12_high(ptr, endptr);
  ptr += 2;
  return val;
}

inline uint16_t
scan_12_low(const uint8_t *ptr, const uint8_t *endptr)
{
  require_distance(ptr, endptr, 2);
  return ((static_cast<uint16_t>(ptr[0]) & 0b0000'1111) << 8) + ptr[1];
}

inline uint16_t
read_12_low(const uint8_t *&ptr, const uint8_t *endptr)
{
  auto val = scan_12_low(ptr, endptr);
  ptr += 2;
  return val;
}

inline uint16_t
scan_16(const uint8_t *ptr, const uint8_t *endptr)
{
  require_distance(ptr, endptr, 2);
  return (static_cast<uint16_t>(ptr[0]) << 8) + static_cast<uint16_t>(ptr[1]);
}

inline uint16_t
read_16(const uint8_t *&ptr, const uint8_t *endptr)
{
  auto val = scan_16(ptr, endptr);
  ptr += 2;
  return val;
}

inline uint32_t
scan_32(const uint8_t *ptr, const uint8_t *endptr)
{
  require_distance(ptr, endptr, 4);
  return (static_cast<uint32_t>(ptr[0]) << 24) + (static_cast<uint32_t>(ptr[1]) << 16) +
         (static_cast<uint32_t>(ptr[2]) << 8) + static_cast<uint32_t>(ptr[3]);
}

inline uint32_t
read_32(const uint8_t *&ptr, const uint8_t *endptr)
{
  auto val = scan_32(ptr, endptr);
  ptr += 4;
  return val;
}

inline uint64_t
scan_33(const uint8_t *ptr, const uint8_t *endptr)
{
  require_distance(ptr, endptr, 5);
  return (static_cast<uint64_t>(ptr[0] & 0b0000'0001) << 32) + (static_cast<uint64_t>(ptr[1]) << 24) +
         (static_cast<uint64_t>(ptr[2]) << 16) + (static_cast<uint64_t>(ptr[3]) << 8) + static_cast<uint64_t>(ptr[4]);
}

inline uint64_t
read_33(const uint8_t *&ptr, const uint8_t *endptr)
{
  auto val = scan_33(ptr, endptr);
  ptr += 5;
  return val;
}

inline uint64_t
scan_48(const uint8_t *ptr, const uint8_t *endptr)
{
  require_distance(ptr, endptr, 6);
  return (static_cast<uint64_t>(read_16(ptr, endptr)) << 32) + scan_32(ptr, endptr);
}

inline uint64_t
read_48(const uint8_t *&ptr, const uint8_t *endptr)
{
  auto val = scan_48(ptr, endptr);
  ptr += 6;
  return val;
}
}
