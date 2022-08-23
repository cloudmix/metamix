#pragma once

#include <cassert>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>

#include "../slice.h"

namespace metamix::h264 {

/**
 * SEI message types
 */
enum SeiType : unsigned int
{
  BUFFERING_PERIOD = 0,       ///< buffering period (H.264, D.1.1)
  PIC_TIMING = 1,             ///< picture timing
  FILLER_PAYLOAD = 3,         ///< filler data
  USER_DATA_REGISTERED = 4,   ///< registered user data as specified by Rec. ITU-T T.35
  USER_DATA_UNREGISTERED = 5, ///< unregistered user data
  RECOVERY_POINT = 6,         ///< recovery point (frame # to decoder sync)
  FRAME_PACKING = 45,         ///< frame packing arrangement
  DISPLAY_ORIENTATION = 47,   ///< display orientation
  GREEN_METADATA = 56,        ///< GreenMPEG information
  ALTERNATIVE_TRANSFER = 147, ///< alternative transfer
  UNDEFINED = std::numeric_limits<unsigned int>::max(),
};

std::string
sei_type_string(SeiType ty);

class SeiPayload : public AbstractSlice<SeiPayload, uint8_t>
{
public:
  ~SeiPayload() override = default;

  virtual SeiType type() const = 0;

  friend std::ostream &operator<<(std::ostream &os, const SeiPayload &sei);
};

class OwnedSeiPayload : public AbstractOwnedSlice<OwnedSeiPayload, SeiPayload>
{
private:
  SeiType m_payload_type = SeiType::UNDEFINED;

public:
  OwnedSeiPayload() = default;

  explicit OwnedSeiPayload(SeiType payload_type, std::vector<uint8_t> data)
    : AbstractOwnedSlice(std::move(data))
    , m_payload_type(payload_type)
  {}

  explicit OwnedSeiPayload(const SeiPayload &base)
    : AbstractOwnedSlice(base)
    , m_payload_type(base.type())
  {}

  OwnedSeiPayload(SeiType payload_type, std::initializer_list<uint8_t> init)
    : AbstractOwnedSlice(init)
    , m_payload_type(payload_type)
  {}

  SeiType type() const override { return m_payload_type; }
};
}
