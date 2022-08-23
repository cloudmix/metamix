#pragma once

#include <cstdint>
#include <cstdlib>
#include <optional>
#include <tuple>

#include "../binary_parser.h"

namespace metamix::h264 {

std::optional<BinaryParserBounds>
nalu_parser_next(const uint8_t *startptr, size_t length);
}
