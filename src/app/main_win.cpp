#include "app/app.hpp"
#include "app/launch_options.hpp"
#include "hax/platform/runtime_policy.hpp"
#include "include/cef_app.h"

#include <windows.h>
#include <shellapi.h>

#include <filesystem>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

namespace {

std::filesystem::path executable_directory() {
  wchar_t path[32768]{};
  const DWORD chars = GetModuleFileNameW(
      nullptr,
      path,
      static_cast<DWORD>(std::size(path)));

  if (chars == 0 || chars >= std::size(path)) {
    return {};
  }

  return std::filesystem::path(path).parent_path();
}

std::filesystem::path profile_path() {
  wchar_t local_app_data[32768]{};
  const DWORD chars = GetEnvironmentVariableW(
      L"LOCALAPPDATA",
      local_app_data,
      static_cast<DWORD>(std::size(local_app_data)));

  if (chars == 0 || chars >= std::size(local_app_data)) {
    return {};
  }

  return std::filesystem::path(local_app_data) /
         L"HaxballApp" /
         L"profile.ini";
}

bool launch_hidden_bootstrap() {
  const auto root = executable_directory();
  if (root.empty()) {
    return false;
  }

  const auto script = root / L"scripts" / L"run.ps1";
  if (!std::filesystem::exists(script)) {
    return false;
  }

  wchar_t system_root[32768]{};
  const DWORD chars = GetEnvironmentVariableW(
      L"SystemRoot",
      system_root,
      static_cast<DWORD>(std::size(system_root)));

  if (chars == 0 || chars >= std::size(system_root)) {
    return false;
  }

  const auto powershell =
      std::filesystem::path(system_root) /
      L"System32" /
      L"WindowsPowerShell" /
      L"v1.0" /
      L"powershell.exe";

  if (!std::filesystem::exists(powershell)) {
    return false;
  }

  std::wstring command =
      L"\"" + powershell.wstring() +
      L"\" -NoProfile -ExecutionPolicy Bypass "
      L"-WindowStyle Hidden -File \"" +
      script.wstring() + L"\"";

  std::vector<wchar_t> mutable_command(
      command.begin(),
      command.end());
  mutable_command.push_back(L'\0');

  STARTUPINFOW startup{};
  startup.cb = sizeof(startup);
  PROCESS_INFORMATION process{};

  const BOOL created = CreateProcessW(
      powershell.c_str(),
      mutable_command.data(),
      nullptr,
      nullptr,
      FALSE,
      CREATE_NO_WINDOW,
      nullptr,
      root.c_str(),
      &startup,
      &process);

  if (!created) {
    return false;
  }

  CloseHandle(process.hThread);
  CloseHandle(process.hProcess);
  return true;
}

int run(HINSTANCE instance) {
  CefMainArgs main_args(instance);
  const auto options = hax::app::parse_launch_options();

  if (!options.benchmark_mode &&
      !options.bootstrap_complete &&
      launch_hidden_bootstrap()) {
    const auto profile = profile_path();
    if (profile.empty() || !std::filesystem::exists(profile)) {
      MessageBoxW(
          nullptr,
          L"İlk açılışta Haxball App bilgisayarınıza göre optimize ediliyor. "
          L"Bu işlem kısa bir süre alabilir; ardından uygulama otomatik açılacak.",
          L"Haxball App",
          MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND);
    }
    return 0;
  }

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
        L"HaxballApp" / L"cef";
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
