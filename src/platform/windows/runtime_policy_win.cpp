#include "hax/platform/runtime_policy.hpp"

#include "hax/platform/system_probe.hpp"

#include <windows.h>

#include <algorithm>
#include <vector>

namespace hax::platform {
namespace {

DWORD priority_class(const hax::core::PriorityPolicy policy) {
  switch (policy) {
    case hax::core::PriorityPolicy::normal:
      return NORMAL_PRIORITY_CLASS;
    case hax::core::PriorityPolicy::above_normal:
      return ABOVE_NORMAL_PRIORITY_CLASS;
    case hax::core::PriorityPolicy::high:
      return HIGH_PRIORITY_CLASS;
  }
  return NORMAL_PRIORITY_CLASS;
}

bool apply_performance_cpu_sets() {
  const auto snapshot = probe_system();
  if (!snapshot.heterogeneous_cpu()) {
    return false;
  }

  std::uint8_t fastest = 0;
  for (const auto& cpu : snapshot.cpu_sets) {
    fastest = std::max(fastest, cpu.efficiency_class);
  }

  std::vector<ULONG> ids;
  for (const auto& cpu : snapshot.cpu_sets) {
    if (!cpu.parked && cpu.efficiency_class == fastest) {
      ids.push_back(cpu.id);
    }
  }
  if (ids.empty()) {
    return false;
  }

  return SetProcessDefaultCpuSets(
             GetCurrentProcess(),
             ids.data(),
             static_cast<ULONG>(ids.size())) != FALSE;
}

bool disable_execution_throttling() {
  PROCESS_POWER_THROTTLING_STATE state{};
  state.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;
  state.ControlMask =
      PROCESS_POWER_THROTTLING_EXECUTION_SPEED |
      PROCESS_POWER_THROTTLING_IGNORE_TIMER_RESOLUTION;
  state.StateMask = 0;

  return SetProcessInformation(
             GetCurrentProcess(),
             ProcessPowerThrottling,
             &state,
             sizeof(state)) != FALSE;
}

}  // namespace

AppliedPolicy apply_runtime_policy(
    const hax::core::CandidateProfile& profile) {
  AppliedPolicy applied;
  applied.priority_applied =
      SetPriorityClass(
          GetCurrentProcess(), priority_class(profile.priority)) != FALSE;

  if (profile.cpu ==
      hax::core::CpuPolicy::prefer_performance_cores) {
    applied.cpu_sets_applied = apply_performance_cpu_sets();
  }

  if (profile.disable_power_throttling ||
      profile.honor_timer_resolution) {
    applied.power_policy_applied = disable_execution_throttling();
  }

  return applied;
}

}  // namespace hax::platform
