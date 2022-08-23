#pragma once

#include <memory>
#include <string>
#include <thread>

#include "../application_context.h"

namespace metamix::proc {

void
injector(std::shared_ptr<ApplicationContext> ctx);
}
