#pragma once

#include "../binary_parser.h"
#include "sei_payload.h"

namespace metamix::h264 {

class SeiParserBounds : public BinaryParserBounds
{
private:
  unsigned int m_payload_type;

public:
  constexpr SeiParserBounds(const uint8_t *startptr, size_t payload_size, unsigned int payload_type)
    : BinaryParserBounds(startptr, payload_size)
    , m_payload_type(payload_type)
  {}

  constexpr unsigned int payload_type() const { return m_payload_type; }
};

std::optional<SeiParserBounds>
sei_parser_next(const uint8_t *startptr, size_t length);

inline OwnedSeiPayload sei_parser_pack([[maybe_unused]] const BinaryParserContext &ctx, const SeiParserBounds &bounds)
{
  assert(bounds.startptr() >= ctx.startptr());
  assert(bounds.length() > 0);
  return OwnedSeiPayload(static_cast<SeiType>(bounds.payload_type()),
                         std::vector<uint8_t>(bounds.startptr(), bounds.startptr() + bounds.length()));
}

// Input: SODB byte array, Output: OwnedSeiPayload
using SeiParser = BinaryParser<OwnedSeiPayload, BinaryParserContext, SeiParserBounds, sei_parser_next, sei_parser_pack>;
}
