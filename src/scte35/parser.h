#pragma once

#include "../binary_parser.h"

#include "scte35.h"

namespace metamix::scte35 {

std::optional<BinaryParserBounds>
scte35_parser_next(const uint8_t *startptr, size_t length);

SpliceInfoSection
scte35_parser_pack(const BinaryParserContext &ctx, const BinaryParserBounds &bounds);

using Scte35Parser =
  BinaryParser<SpliceInfoSection, BinaryParserContext, BinaryParserBounds, scte35_parser_next, scte35_parser_pack>;
}
