#include "util.h"

#include <cstdio>

#include "ffmpeg.h"

namespace metamix {

void
hex_dump(const uint8_t *buffer, size_t size)
{
  av_hex_dump(stderr, buffer, size);
}
}
