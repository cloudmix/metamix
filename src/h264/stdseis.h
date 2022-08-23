#pragma once

#include <memory>

#include "../clock_types.h"
#include "../iospec.h"
#include "../metadata.h"

#include "sei_payload.h"

namespace metamix::h264 {

Metadata<SeiKind>
build_empty_metadata(InputId input_id, ClockTS ts);

Metadata<SeiKind>
build_cc_reset_metadata(InputId input_id, ClockTS ts);
}
