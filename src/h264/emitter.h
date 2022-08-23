#pragma once

#include <cassert>
#include <iterator>

#include <boost/endian/conversion.hpp>

#include "nalu.h"
#include "rbsp.h"
#include "sei_payload.h"

namespace metamix::h264 {

namespace detail {

using boost::endian::native_to_big_inplace;

constexpr uint8_t SEI_NALU_TYPE_BYTE = static_cast<uint8_t>(NaluType::SEI);

constexpr unsigned int
variadic_length_int_size(unsigned int num)
{
  return num / 255 + 1;
}

template<typename OutputIt>
OutputIt
emit_variadic_length_int(unsigned int num, OutputIt dest)
{
  while (num >= 255) {
    *dest = 0xFF;
    dest++;
    num /= 255;
  }

  *dest = num % 255;
  dest++;

  return dest;
}

template<class OutputIt, unsigned int NaluLengthSize = 4>
OutputIt
emit_nalu_length(uint32_t nalu_length, OutputIt dest)
{
  static_assert(0 < NaluLengthSize && NaluLengthSize <= sizeof(uint32_t));

  assert(0 <= nalu_length && nalu_length < Nalu::MAX_LENGTH);

  union
  {
    uint32_t number;
    uint8_t array[sizeof(uint32_t)];
  } result{ nalu_length };

  native_to_big_inplace(result.number);

  return std::copy(result.array + sizeof(result) - NaluLengthSize, result.array + sizeof(result), dest);
}
}

uint32_t
sei_payload_size_hint(const SeiPayload &sei)
{
  uint32_t x = 0;

  // RBSP payload size, it is not possible to get emulation byte in payload type/size,
  // so we don't count them
  x += sei.size() + count_emulation_prevention_bytes(sei.cbegin(), sei.cend()) + 1;

  // Payload length size
  x += detail::variadic_length_int_size(static_cast<unsigned int>(sei.size()));

  // Payload type size
  x += detail::variadic_length_int_size(sei.type());

  return x;
}

template<class OutputIt>
OutputIt
emit_sei_payload(const SeiPayload &sei, OutputIt dest)
{
  dest = detail::emit_variadic_length_int(sei.type(), dest);
  dest = detail::emit_variadic_length_int(static_cast<unsigned int>(sei.size()), dest);
  return copy_sodb_to_ebsp(sei.cbegin(), sei.cend(), dest);
}

template<class InputIt, class OutputIt, unsigned int NaluLengthSize = 4>
OutputIt
emit_sei_payloads_to_avcc_nalu(InputIt from, InputIt to, OutputIt dest)
{
  static_assert(
    std::is_base_of<std::random_access_iterator_tag, typename std::iterator_traits<InputIt>::iterator_category>::value,
    "input iterator must be of random access iterator category");
  static_assert(
    std::is_base_of<std::output_iterator_tag, typename std::iterator_traits<OutputIt>::iterator_category>::value,
    "output iterator must be of output iterator category");

  auto size_hint = 1;
  for (auto it = from; it != to; it++) {
    size_hint += sei_payload_size_hint(*it);
  }

  dest = detail::emit_nalu_length<OutputIt, NaluLengthSize>(size_hint, dest);

  *dest = detail::SEI_NALU_TYPE_BYTE;
  dest++;

  for (auto it = from; it != to; it++) {
    dest = emit_sei_payload(*it, dest);
  }

  return dest;
}

template<class OutputIt, unsigned int NaluLengthSize = 4>
OutputIt
emit_avcc_nalu(const Nalu &nalu, OutputIt dest)
{
  dest = detail::emit_nalu_length<OutputIt, NaluLengthSize>(static_cast<uint32_t>(nalu.size()), dest);
  return std::copy(nalu.cbegin(), nalu.cend(), dest);
}
}
