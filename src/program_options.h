#pragma once

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "iospec.h"
#include "log.h"

namespace metamix {

struct ProgramOptions
{
  std::vector<InputSpec> user_inputs{};
  OutputSpec output{};

  std::optional<std::string> start_input_name = std::nullopt;

  std::string http_address{};
  unsigned short http_port{ 0 };

  boost::log::trivial::severity_level logging_level{ boost::log::trivial::info };
  std::optional<std::string> logging_thread{};

  bool norestart{ false };

  static ProgramOptions *parse(int argc, char *argv[]);

  void validate() const;
};
}
