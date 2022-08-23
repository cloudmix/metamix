#pragma once

#include <memory>
#include <string>
#include <thread>

#include "../application_context.h"

namespace metamix::proc {

void
controller(std::shared_ptr<ApplicationContext> ctx);
}
