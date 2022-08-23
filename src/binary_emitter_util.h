#pragma once

#include <cassert>
#include <cstdint>
#include <iterator>
#include <string>

namespace metamix {

template<class OutputIt>
inline OutputIt
write_8(uint8_t value, OutputIt out)
{
  static_assert(
    std::is_base_of<std::output_iterator_tag, typename std::iterator_traits<OutputIt>::iterator_category>::value,
    "output iterator must be of output iterator category");

  *out++ = value;
  return out;
}

template<class OutputIt>
inline OutputIt
write_12_prefix(uint8_t prefix, uint16_t value, OutputIt out)
{
  static_assert(
    std::is_base_of<std::output_iterator_tag, typename std::iterator_traits<OutputIt>::iterator_category>::value,
    "output iterator must be of output iterator category");

  *out++ = static_cast<uint8_t>(((prefix & 0x0f) << 4) | ((value & 0x0f00) >> 8));
  *out++ = static_cast<uint8_t>((value & 0x00ff));
  return out;
}

template<class OutputIt>
inline OutputIt
write_12_pair(uint16_t high, uint16_t low, OutputIt out)
{
  static_assert(
    std::is_base_of<std::output_iterator_tag, typename std::iterator_traits<OutputIt>::iterator_category>::value,
    "output iterator must be of output iterator category");

  *out++ = static_cast<uint8_t>((high & 0x0ff0) >> 4);
  *out++ = static_cast<uint8_t>((high & 0x000f) << 4) | static_cast<uint8_t>((low & 0x0f00 >> 4));
  *out++ = static_cast<uint8_t>(low & 0x00ff);
  return out;
}

template<class OutputIt>
inline OutputIt
write_16(uint16_t value, OutputIt out)
{
  static_assert(
    std::is_base_of<std::output_iterator_tag, typename std::iterator_traits<OutputIt>::iterator_category>::value,
    "output iterator must be of output iterator category");

  *out++ = static_cast<uint8_t>((value & 0xff00) >> 8);
  *out++ = static_cast<uint8_t>((value & 0x00ff));
  return out;
}

template<class OutputIt>
inline OutputIt
write_32(uint32_t value, OutputIt out)
{
  static_assert(
    std::is_base_of<std::output_iterator_tag, typename std::iterator_traits<OutputIt>::iterator_category>::value,
    "output iterator must be of output iterator category");

  *out++ = static_cast<uint8_t>((value & 0xff000000) >> 24);
  *out++ = static_cast<uint8_t>((value & 0x00ff0000) >> 16);
  *out++ = static_cast<uint8_t>((value & 0x0000ff00) >> 8);
  *out++ = static_cast<uint8_t>((value & 0x000000ff));
  return out;
}

template<class OutputIt>
inline OutputIt
write_33_prefix(uint8_t prefix, uint64_t value, OutputIt out)
{
  static_assert(
    std::is_base_of<std::output_iterator_tag, typename std::iterator_traits<OutputIt>::iterator_category>::value,
    "output iterator must be of output iterator category");

  out = write_8((prefix & 0xfe) | ((value & 0x100000000) >> 32), out);
  out = write_32(value & 0xffffffff, out);
  return out;
}

template<class OutputIt>
inline OutputIt
write_48(uint64_t value, OutputIt out)
{
  static_assert(
    std::is_base_of<std::output_iterator_tag, typename std::iterator_traits<OutputIt>::iterator_category>::value,
    "output iterator must be of output iterator category");

  *out++ = static_cast<uint8_t>((value & 0xff0000000000) >> 40);
  *out++ = static_cast<uint8_t>((value & 0x00ff00000000) >> 32);
  *out++ = static_cast<uint8_t>((value & 0x0000ff000000) >> 24);
  *out++ = static_cast<uint8_t>((value & 0x000000ff0000) >> 16);
  *out++ = static_cast<uint8_t>((value & 0x00000000ff00) >> 8);
  *out++ = static_cast<uint8_t>((value & 0x0000000000ff));
  return out;
}
}
