#include "app/app.hpp"
#include "app/launch_options.hpp"
#include "hax/platform/runtime_policy.hpp"
#include "include/cef_app.h"

#include <windows.h>

#include <filesystem>
#include <iterator>

namespace {

int run(HINSTANCE instance) {
  CefMainArgs main_args(instance);
  const auto options = hax::app::parse_launch_options();

  const auto applied =
      hax::platform::apply_runtime_policy(options.profile);

  if (options.benchmark_mode) {
    const bool priority_ok = applied.priority_applied;
    const bool cpu_ok =
        options.profile.cpu !=
            hax::core::CpuPolicy::prefer_performance_cores ||
        applied.cpu_sets_applied;
    const bool power_ok =
        (!options.profile.disable_power_throttling &&
         !options.profile.honor_timer_resolution) ||
        applied.power_policy_applied;

    if (!priority_ok || !cpu_ok || !power_ok) {
      return 3;
    }
  }

  CefRefPtr<hax::app::App> app(new hax::app::App(options));

  void* sandbox_info = nullptr;
  const int child_exit = CefExecuteProcess(main_args, app, sandbox_info);
  if (child_exit >= 0) {
    return child_exit;
  }

  CefSettings settings;
  settings.no_sandbox = true;
  settings.log_severity = LOGSEVERITY_WARNING;
  settings.persist_session_cookies = true;
  settings.windowless_rendering_enabled = false;

  wchar_t local_app_data[32768]{};
  const DWORD chars = GetEnvironmentVariableW(
      L"LOCALAPPDATA",
      local_app_data,
      static_cast<DWORD>(std::size(local_app_data)));

  if (chars > 0 && chars < std::size(local_app_data)) {
    const auto cache =
        std::filesystem::path(local_app_data) /
        L"HaxPerformanceRuntime" / L"cef";
    std::error_code error;
    std::filesystem::create_directories(cache, error);
    if (!error) {
      CefString(&settings.root_cache_path) = cache.wstring();
    }
  }

  if (!CefInitialize(main_args, settings, app, sandbox_info)) {
    return 1;
  }

  CefRunMessageLoop();
  CefShutdown();
  return 0;
}

}  // namespace

int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, LPWSTR, int) {
  (void)SetProcessDpiAwarenessContext(
      DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
  return run(instance);
}
