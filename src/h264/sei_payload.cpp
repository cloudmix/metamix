#include "sei_payload.h"

#include <boost/core/demangle.hpp>
#include <boost/format.hpp>

namespace metamix::h264 {

std::string
sei_type_string(SeiType ty)
{
  switch (ty) {
  case SeiType::BUFFERING_PERIOD:
    return "BUFFERING_PERIOD";
  case SeiType::PIC_TIMING:
    return "PIC_TIMING";
  case SeiType::FILLER_PAYLOAD:
    return "FILLER_PAYLOAD";
  case SeiType::USER_DATA_REGISTERED:
    return "USER_DATA_REGISTERED";
  case SeiType::USER_DATA_UNREGISTERED:
    return "USER_DATA_UNREGISTERED";
  case SeiType::RECOVERY_POINT:
    return "RECOVERY_POINT";
  case SeiType::FRAME_PACKING:
    return "FRAME_PACKING";
  case SeiType::DISPLAY_ORIENTATION:
    return "DISPLAY_ORIENTATION";
  case SeiType::GREEN_METADATA:
    return "GREEN_METADATA";
  case SeiType::ALTERNATIVE_TRANSFER:
    return "ALTERNATIVE_TRANSFER";
  case SeiType::UNDEFINED:
    return "UNDEFINED";
  default: {
    auto fmt = boost::format("UNSPECIFIED%1%") % static_cast<unsigned int>(ty);
    return fmt.str();
  }
  }
}

std::ostream &
operator<<(std::ostream &os, const SeiPayload &sei)
{
  os << boost::core::demangle(typeid(sei).name()) << "{";
  if (!sei.empty()) {
    os << "type=" << sei_type_string(sei.type()) << ","
       << "data=0x" << std::hex << reinterpret_cast<size_t>(sei.data()) << std::dec << ","
       << "size=" << sei.size();
  } else {
    os << "0-sized";
  }
  os << "}";
  return os;
}
}
