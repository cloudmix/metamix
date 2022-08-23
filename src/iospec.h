#pragma once

#include <optional>
#include <string>

namespace metamix {

using InputId = unsigned int;

struct InputSpec
{
  InputId id{ 0 };
  std::string name{};
  std::string source{};
  std::string sink{};
  std::optional<std::string> source_format{ std::nullopt };
  std::optional<std::string> sink_format{ std::nullopt };
  bool is_virtual{ false };
};

struct OutputSpec
{
  std::string source{};
  std::string sink{};
  std::optional<std::string> source_format{ std::nullopt };
  std::optional<std::string> sink_format{ std::nullopt };
  int64_t ts_adjustment{ 0 };
};
}
