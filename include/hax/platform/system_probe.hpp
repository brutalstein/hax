#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace hax::platform {

struct CpuSet {
  std::uint32_t id{0};
  std::uint16_t group{0};
  std::uint8_t logical_index{0};
  std::uint8_t core_index{0};
  std::uint8_t llc_index{0};
  std::uint8_t numa_index{0};
  std::uint8_t efficiency_class{0};
  bool parked{false};
};

struct GpuAdapter {
  std::string name;
  std::uint64_t dedicated_video_memory{0};
  bool software{false};
};

struct DisplayInfo {
  std::uint32_t width{0};
  std::uint32_t height{0};
  double refresh_hz{0.0};
};

struct SystemSnapshot {
  std::vector<CpuSet> cpu_sets;
  std::vector<GpuAdapter> gpus;
  DisplayInfo primary_display;
  bool on_ac_power{true};

  [[nodiscard]] bool heterogeneous_cpu() const;
};

[[nodiscard]] SystemSnapshot probe_system();

}  // namespace hax::platform
