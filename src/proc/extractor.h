#pragma once

#include <memory>
#include <string>
#include <thread>

#include "../application_context.h"

namespace metamix::proc {

void
extractor(std::string input_name, std::shared_ptr<ApplicationContext> ctx);
}
