#include "hax/platform/system_probe.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iostream>

int main() {
  const auto system = hax::platform::probe_system();

  const auto hardware_gpu_count = std::count_if(
      system.gpus.begin(),
      system.gpus.end(),
      [](const hax::platform::GpuAdapter& gpu) {
        return !gpu.software;
      });

  std::cout
      << "HETEROGENEOUS_CPU="
      << (system.heterogeneous_cpu() ? 1 : 0)
      << '\n'
      << "CPU_SET_COUNT="
      << system.cpu_sets.size()
      << '\n'
      << "GPU_COUNT="
      << hardware_gpu_count
      << '\n'
      << "REFRESH_HZ="
      << static_cast<long long>(
             std::llround(system.primary_display.refresh_hz))
      << '\n'
      << "DISPLAY_WIDTH="
      << system.primary_display.width
      << '\n'
      << "DISPLAY_HEIGHT="
      << system.primary_display.height
      << '\n'
      << "ON_AC="
      << (system.on_ac_power ? 1 : 0)
      << '\n';

  for (std::size_t i = 0; i < system.gpus.size(); ++i) {
    const auto& gpu = system.gpus[i];
    std::cout
        << "GPU_" << i << "_NAME=" << gpu.name
        << '\n'
        << "GPU_" << i << "_VRAM="
        << gpu.dedicated_video_memory
        << '\n'
        << "GPU_" << i << "_SOFTWARE="
        << (gpu.software ? 1 : 0)
        << '\n';
  }

  return 0;
}
