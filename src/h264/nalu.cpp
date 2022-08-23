#include "nalu.h"

#include <array>

#include <boost/algorithm/string.hpp>
#include <boost/core/demangle.hpp>

namespace metamix::h264 {

constexpr std::array<const char *, 32> NALU_TYPE_STR{
  // clang-format off
  "UNSPECIFIED",
  "SLICE",
  "DPA",
  "DPB",
  "DPC",
  "IDR_SLICE",
  "SEI",
  "SPS",
  "PPS",
  "AUD",
  "END_SEQUENCE",
  "END_STREAM",
  "FILLER_DATA",
  "SPS_EXT",
  "PREFIX",
  "SUB_SPS",
  "DPS",
  "RESERVED17",
  "RESERVED18",
  "AUXILIARY_SLICE",
  "EXTEN_SLICE",
  "DEPTH_EXTEN_SLICE",
  "RESERVED22",
  "RESERVED23",
  "UNSPECIFIED24",
  "UNSPECIFIED25",
  "UNSPECIFIED26",
  "UNSPECIFIED27",
  "UNSPECIFIED28",
  "UNSPECIFIED29",
  "UNSPECIFIED30",
  "UNSPECIFIED31",
  // clang-format on
};

const char *
nalu_type_to_string(NaluType ty)
{
  return NALU_TYPE_STR[static_cast<size_t>(ty)];
}

std::optional<NaluType>
string_to_nalu_type(const char *str)
{
  std::string upper(str);
  boost::to_upper(upper);

  for (unsigned int i = 0; i < NALU_TYPE_STR.size(); ++i) {
    if (upper == NALU_TYPE_STR[i]) {
      return static_cast<NaluType>(i);
    }
  }

  return std::nullopt;
}

std::ostream &
operator<<(std::ostream &os, const NaluType &ty)
{
  return os << nalu_type_to_string(ty);
}

std::ostream &
operator<<(std::ostream &os, const Nalu &nalu)
{
  os << boost::core::demangle(typeid(nalu).name()) << "{";
  if (!nalu.empty()) {
    os << "type=" << nalu.type() << ","
       << "data=0x" << std::hex << reinterpret_cast<size_t>(nalu.data()) << std::dec << ","
       << "size=" << nalu.size();
  } else {
    os << "0-sized";
  }
  os << "}";
  return os;
}
}
