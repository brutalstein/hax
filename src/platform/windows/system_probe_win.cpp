#include "hax/platform/system_probe.hpp"

#include <windows.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <cstddef>
#include <set>
#include <string>
#include <vector>

namespace hax::platform {
namespace {

std::string narrow(const wchar_t* input) {
  if (input == nullptr || *input == L'\0') {
    return {};
  }
  const int required =
      WideCharToMultiByte(
          CP_UTF8, 0, input, -1, nullptr, 0, nullptr, nullptr);
  if (required <= 1) {
    return {};
  }

  std::string out(static_cast<std::size_t>(required), '\0');
  WideCharToMultiByte(
      CP_UTF8, 0, input, -1, out.data(), required, nullptr, nullptr);
  if (!out.empty() && out.back() == '\0') {
    out.pop_back();
  }
  return out;
}

std::vector<CpuSet> probe_cpu_sets() {
  ULONG bytes = 0;
  GetSystemCpuSetInformation(
      nullptr, 0, &bytes, GetCurrentProcess(), 0);
  if (bytes == 0) {
    return {};
  }

  std::vector<std::byte> buffer(bytes);
  if (!GetSystemCpuSetInformation(
          reinterpret_cast<PSYSTEM_CPU_SET_INFORMATION>(buffer.data()),
          bytes,
          &bytes,
          GetCurrentProcess(),
          0)) {
    return {};
  }

  std::vector<CpuSet> result;
  std::size_t offset = 0;
  while (offset < bytes) {
    const auto* info =
        reinterpret_cast<const SYSTEM_CPU_SET_INFORMATION*>(
            buffer.data() + offset);
    if (info->Size == 0) {
      break;
    }

    if (info->Type == CpuSetInformation) {
      const auto& c = info->CpuSet;
      result.push_back(CpuSet{
          c.Id,
          c.Group,
          c.LogicalProcessorIndex,
          c.CoreIndex,
          c.LastLevelCacheIndex,
          c.NumaNodeIndex,
          c.EfficiencyClass,
          c.Parked != 0,
      });
    }
    offset += info->Size;
  }
  return result;
}

std::vector<GpuAdapter> probe_gpus() {
  using Microsoft::WRL::ComPtr;

  ComPtr<IDXGIFactory6> factory;
  if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) {
    return {};
  }

  std::vector<GpuAdapter> result;
  for (UINT index = 0;; ++index) {
    ComPtr<IDXGIAdapter1> adapter;
    const HRESULT hr = factory->EnumAdapterByGpuPreference(
        index,
        DXGI_GPU_PREFERENCE_UNSPECIFIED,
        IID_PPV_ARGS(&adapter));
    if (hr == DXGI_ERROR_NOT_FOUND) {
      break;
    }
    if (FAILED(hr)) {
      break;
    }

    DXGI_ADAPTER_DESC1 desc{};
    if (SUCCEEDED(adapter->GetDesc1(&desc))) {
      result.push_back(GpuAdapter{
          narrow(desc.Description),
          static_cast<std::uint64_t>(desc.DedicatedVideoMemory),
          (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0,
      });
    }
  }
  return result;
}

DisplayInfo probe_primary_display() {
  DEVMODEW mode{};
  mode.dmSize = sizeof(mode);
  if (!EnumDisplaySettingsW(
          nullptr, ENUM_CURRENT_SETTINGS, &mode)) {
    return {};
  }

  return DisplayInfo{
      mode.dmPelsWidth,
      mode.dmPelsHeight,
      static_cast<double>(mode.dmDisplayFrequency),
  };
}

bool probe_ac_power() {
  SYSTEM_POWER_STATUS status{};
  if (!GetSystemPowerStatus(&status)) {
    return true;
  }
  return status.ACLineStatus != 0;
}

}  // namespace

bool SystemSnapshot::heterogeneous_cpu() const {
  std::set<std::uint8_t> classes;
  for (const auto& cpu : cpu_sets) {
    classes.insert(cpu.efficiency_class);
  }
  return classes.size() > 1;
}

SystemSnapshot probe_system() {
  SystemSnapshot snapshot;
  snapshot.cpu_sets = probe_cpu_sets();
  snapshot.gpus = probe_gpus();
  snapshot.primary_display = probe_primary_display();
  snapshot.on_ac_power = probe_ac_power();
  return snapshot;
}

}  // namespace hax::platform
