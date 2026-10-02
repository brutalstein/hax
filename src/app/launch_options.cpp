#include "app/launch_options.hpp"

#include <windows.h>
#include <shellapi.h>

#include <filesystem>
#include <string_view>

namespace hax::app {

LaunchOptions parse_launch_options() {
  LaunchOptions options;

  int argc = 0;
  LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
  if (argv == nullptr) {
    return options;
  }

  for (int i = 1; i < argc; ++i) {
    const std::wstring_view arg(argv[i]);

    if (arg == L"--hax-benchmark") {
      options.benchmark_mode = true;
    } else if (arg == L"--hax-fps=default") {
      options.profile.frame =
          hax::core::FramePolicy::browser_default;
      options.profile.disable_vsync = false;
    } else if (arg == L"--hax-fps=uncapped") {
      options.profile.frame = hax::core::FramePolicy::uncapped;
      options.profile.disable_vsync = true;
    } else if (arg == L"--hax-gpu=high") {
      options.profile.gpu =
          hax::core::GpuPreference::high_performance;
    } else if (arg == L"--hax-gpu=low") {
      options.profile.gpu = hax::core::GpuPreference::low_power;
    } else if (arg == L"--hax-cpu=performance") {
      options.profile.cpu =
          hax::core::CpuPolicy::prefer_performance_cores;
    } else if (arg == L"--hax-priority=normal") {
      options.profile.priority =
          hax::core::PriorityPolicy::normal;
    } else if (arg == L"--hax-priority=high") {
      options.profile.priority =
          hax::core::PriorityPolicy::high;
    } else if (arg.starts_with(L"--hax-url=")) {
      const std::wstring wide(arg.substr(10));
      const int bytes = WideCharToMultiByte(
          CP_UTF8,
          0,
          wide.data(),
          static_cast<int>(wide.size()),
          nullptr,
          0,
          nullptr,
          nullptr);

      if (bytes > 0) {
        options.url.resize(static_cast<std::size_t>(bytes));
        WideCharToMultiByte(
            CP_UTF8,
            0,
            wide.data(),
            static_cast<int>(wide.size()),
            options.url.data(),
            bytes,
            nullptr,
            nullptr);
      }
    }
  }

  LocalFree(argv);

  if (options.benchmark_mode) {
    wchar_t path[MAX_PATH]{};
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    options.benchmark_path =
        (std::filesystem::path(path).parent_path() /
         L"benchmark.html").wstring();
  }

  return options;
}

}  // namespace hax::app
