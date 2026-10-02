#include "app/launch_options.hpp"

#include <windows.h>
#include <shellapi.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

namespace hax::app {
namespace {

void apply_setting(
    LaunchOptions& options,
    const std::wstring_view key,
    const std::wstring_view value) {
  if (key == L"frame") {
    if (value == L"default") {
      options.profile.frame = hax::core::FramePolicy::browser_default;
      options.profile.disable_vsync = false;
    } else if (value == L"uncapped") {
      options.profile.frame = hax::core::FramePolicy::uncapped;
      options.profile.disable_vsync = true;
    }
  } else if (key == L"gpu") {
    if (value == L"default") {
      options.profile.gpu = hax::core::GpuPreference::system_default;
    } else if (value == L"low") {
      options.profile.gpu = hax::core::GpuPreference::low_power;
    } else if (value == L"high") {
      options.profile.gpu = hax::core::GpuPreference::high_performance;
    }
  } else if (key == L"cpu") {
    if (value == L"default") {
      options.profile.cpu = hax::core::CpuPolicy::system_default;
    } else if (value == L"performance") {
      options.profile.cpu =
          hax::core::CpuPolicy::prefer_performance_cores;
    }
  } else if (key == L"priority") {
    if (value == L"normal") {
      options.profile.priority = hax::core::PriorityPolicy::normal;
    } else if (value == L"above") {
      options.profile.priority = hax::core::PriorityPolicy::above_normal;
    } else if (value == L"high") {
      options.profile.priority = hax::core::PriorityPolicy::high;
    }
  }
}

std::filesystem::path persisted_profile_path() {
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

void load_persisted_profile(LaunchOptions& options) {
  const auto path = persisted_profile_path();
  if (path.empty()) {
    return;
  }

  std::wifstream input(path);
  if (!input) {
    return;
  }

  std::wstring line;
  while (std::getline(input, line)) {
    if (line.empty() || line.front() == L'#') {
      continue;
    }

    const auto equals = line.find(L'=');
    if (equals == std::wstring::npos) {
      continue;
    }

    apply_setting(
        options,
        std::wstring_view(line).substr(0, equals),
        std::wstring_view(line).substr(equals + 1));
  }
}

void apply_command_line_setting(
    LaunchOptions& options,
    const std::wstring_view arg) {
  if (arg == L"--hax-fps=default") {
    apply_setting(options, L"frame", L"default");
  } else if (arg == L"--hax-fps=uncapped") {
    apply_setting(options, L"frame", L"uncapped");
  } else if (arg == L"--hax-gpu=default") {
    apply_setting(options, L"gpu", L"default");
  } else if (arg == L"--hax-gpu=high") {
    apply_setting(options, L"gpu", L"high");
  } else if (arg == L"--hax-gpu=low") {
    apply_setting(options, L"gpu", L"low");
  } else if (arg == L"--hax-cpu=default") {
    apply_setting(options, L"cpu", L"default");
  } else if (arg == L"--hax-cpu=performance") {
    apply_setting(options, L"cpu", L"performance");
  } else if (arg == L"--hax-priority=normal") {
    apply_setting(options, L"priority", L"normal");
  } else if (arg == L"--hax-priority=above") {
    apply_setting(options, L"priority", L"above");
  } else if (arg == L"--hax-priority=high") {
    apply_setting(options, L"priority", L"high");
  }
}

}  // namespace

LaunchOptions parse_launch_options() {
  LaunchOptions options;

  int argc = 0;
  LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
  if (argv == nullptr) {
    return options;
  }

  bool ignore_persisted_profile = false;
  for (int i = 1; i < argc; ++i) {
    if (std::wstring_view(argv[i]) == L"--hax-no-profile") {
      ignore_persisted_profile = true;
      break;
    }
  }

  if (!ignore_persisted_profile) {
    load_persisted_profile(options);
  }

  for (int i = 1; i < argc; ++i) {
    const std::wstring_view arg(argv[i]);

    if (arg == L"--hax-benchmark") {
      options.benchmark_mode = true;
      continue;
    }

    if (arg == L"--hax-bootstrap-complete") {
      options.bootstrap_complete = true;
      continue;
    }

    apply_command_line_setting(options, arg);

    if (arg.starts_with(L"--hax-url=")) {
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
