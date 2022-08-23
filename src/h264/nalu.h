#pragma once

#include <cassert>
#include <cstdint>
#include <optional>
#include <ostream>
#include <vector>

#include "../slice.h"

namespace metamix::h264 {

/*
 * Table 7-1 – NAL unit type codes, syntax element categories, and NAL unit type classes in
 * T-REC-H.264-201704
 */
enum class NaluType : uint8_t
{
  UNSPECIFIED = 0,
  SLICE = 1,
  DPA = 2,
  DPB = 3,
  DPC = 4,
  IDR_SLICE = 5,
  SEI = 6,
  SPS = 7,
  PPS = 8,
  AUD = 9,
  END_SEQUENCE = 10,
  END_STREAM = 11,
  FILLER_DATA = 12,
  SPS_EXT = 13,
  PREFIX = 14,
  SUB_SPS = 15,
  DPS = 16,
  RESERVED17 = 17,
  RESERVED18 = 18,
  AUXILIARY_SLICE = 19,
  EXTEN_SLICE = 20,
  DEPTH_EXTEN_SLICE = 21,
  RESERVED22 = 22,
  RESERVED23 = 23,
  UNSPECIFIED24 = 24,
  UNSPECIFIED25 = 25,
  UNSPECIFIED26 = 26,
  UNSPECIFIED27 = 27,
  UNSPECIFIED28 = 28,
  UNSPECIFIED29 = 29,
  UNSPECIFIED30 = 30,
  UNSPECIFIED31 = 31,
};

const char *
nalu_type_to_string(NaluType ty);

std::optional<NaluType>
string_to_nalu_type(const char *str);

inline std::optional<NaluType>
string_to_nalu_type(const std::string &str)
{
  return string_to_nalu_type(str.c_str());
}

std::ostream &
operator<<(std::ostream &os, const NaluType &ty);

class Nalu : public AbstractSlice<Nalu, uint8_t>
{
public:
  static constexpr size_t MAX_LENGTH = 4 * 1024 * 2014;

public:
  ~Nalu() override = default;

  bool is_valid() const noexcept { return !empty() > 0 && (front() & 0b1'00'00000) == 0b0'00'00000; }

  NaluType type() const noexcept { return static_cast<NaluType>(front() & 0b0'00'11111); }

  friend std::ostream &operator<<(std::ostream &os, const Nalu &nalu);
};

class OwnedNalu : public AbstractOwnedSlice<OwnedNalu, Nalu>
{
  using AbstractOwnedSlice::AbstractOwnedSlice;
};
}
