#pragma once

#include "hax/core/model.hpp"

#include <string>

namespace hax::app {

struct LaunchOptions {
  hax::core::CandidateProfile profile;
  bool benchmark_mode{false};
  std::wstring benchmark_path;
  std::string url{"https://www.haxball.com/play"};
};

[[nodiscard]] LaunchOptions parse_launch_options();

}  // namespace hax::app
