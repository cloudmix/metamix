#pragma once

#include <algorithm>
#include <cassert>
#include <iterator>
#include <stdexcept>

namespace metamix::h264 {

namespace detail {

template<bool DropStopBit, class InputIt, class OutputIt>
OutputIt
copy_from_ebsp_impl(InputIt first, InputIt last, OutputIt dest)
{
  static_assert(
    std::is_base_of<std::random_access_iterator_tag, typename std::iterator_traits<InputIt>::iterator_category>::value,
    "input iterator must be of random access iterator category");
  static_assert(
    std::is_base_of<std::output_iterator_tag, typename std::iterator_traits<OutputIt>::iterator_category>::value,
    "output iterator must be of output iterator category");

  if (first == last) {
    return dest;
  }

  // Skip stop bit if requested
  if constexpr (DropStopBit) {
    while (last[-1] == 0) {
      last--;
    }

    if (last[-1] != 0x80) {
      throw std::runtime_error("malformed RBSP payload, missing stop bit");
    }

    last--;
  }

  // If payload is to short, just copy it and return
  if (std::distance(first, last) <= 2) {
    return std::copy(first, last, dest);
  }

  // Copy first two bytes
  dest = std::copy(first, first + 2, dest);
  first += 2;

  while (first < last) {
    // Check whether we are on emulation prevention byte (00 00 <here> 03)
    if (first[-2] == 0 && first[-1] == 0 && first[0] == 3) {
      first++;
    } else {
      *dest = *first;
      first++;
      dest++;
    }
  }

  return dest;
}

template<bool AddStopBit, class InputIt, class OutputIt>
OutputIt
copy_to_ebsp_impl(InputIt first, InputIt last, OutputIt dest)
{
  static_assert(
    std::is_base_of<std::random_access_iterator_tag, typename std::iterator_traits<InputIt>::iterator_category>::value,
    "input iterator must be of random access iterator category");
  static_assert(
    std::is_base_of<std::output_iterator_tag, typename std::iterator_traits<OutputIt>::iterator_category>::value,
    "output iterator must be of output iterator category");

  if (first == last) {
    return dest;
  }

  if (std::distance(first, last) <= 2) {
    dest = std::copy(first, last, dest);
  } else {
    // Copy first two bytes
    dest = std::copy(first, first + 2, dest);
    first += 2;

    while (first < last) {
      // Check whether we are on emulation byte (00 00 <here> 00-03)
      if (first[-2] == 0 && first[-1] == 0 && first[0] <= 3) {
        // Insert emulation prevention byte
        *dest = 0x03;
        dest++;
      }

      *dest = *first;
      first++;
      dest++;
    }
  }

  // Add stop bit if requested
  if constexpr (AddStopBit) {
    *dest = 0x80;
    dest++;
  }

  return dest;
}
}

template<class Iter>
unsigned int
count_emulation_prevention_bytes(Iter first, Iter last)
{
  if (std::distance(first, last) <= 2) {
    return 0;
  }

  unsigned int count = 0;

  first += 2;

  while (first < last) {
    // Check whether we are on emulation prevention byte (00 00 <here> 03)
    if (first[-2] == 0 && first[-1] == 0 && first[0] == 3) {
      count++;
    }

    first++;
  }

  return count;
}

template<class InputIt, class OutputIt>
OutputIt
copy_ebsp_to_rbsp(InputIt srcbeg, InputIt srcend, OutputIt dstbeg)
{
  return detail::copy_from_ebsp_impl<false>(srcbeg, srcend, dstbeg);
}

template<class InputIt, class OutputIt>
OutputIt
copy_ebsp_to_sodb(InputIt srcbeg, InputIt srcend, OutputIt dstbeg)
{
  return detail::copy_from_ebsp_impl<true>(srcbeg, srcend, dstbeg);
}

template<class InputIt, class OutputIt>
OutputIt
copy_rbsp_to_ebsp(InputIt srcbeg, InputIt srcend, OutputIt dstbeg)
{
  return detail::copy_to_ebsp_impl<false>(srcbeg, srcend, dstbeg);
}

template<class InputIt, class OutputIt>
OutputIt
copy_sodb_to_ebsp(InputIt srcbeg, InputIt srcend, OutputIt dstbeg)
{
  return detail::copy_to_ebsp_impl<true>(srcbeg, srcend, dstbeg);
}
}
