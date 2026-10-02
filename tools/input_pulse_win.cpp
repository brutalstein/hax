#include <windows.h>

#include <algorithm>
#include <chrono>
#include <string>
#include <string_view>
#include <thread>

namespace {

int parse_int_arg(
    const int argc,
    wchar_t** argv,
    const std::wstring_view prefix,
    const int fallback) {
  for (int i = 1; i < argc; ++i) {
    const std::wstring_view arg(argv[i]);
    if (!arg.starts_with(prefix)) {
      continue;
    }

    try {
      return std::stoi(std::wstring(arg.substr(prefix.size())));
    } catch (...) {
      return fallback;
    }
  }
  return fallback;
}

bool focus_benchmark_window() {
  constexpr wchar_t kTitle[] = L"Hax Performance Runtime";

  for (int attempt = 0; attempt < 60; ++attempt) {
    HWND window = FindWindowW(nullptr, kTitle);
    if (window != nullptr) {
      ShowWindow(window, SW_RESTORE);
      BringWindowToTop(window);
      if (SetForegroundWindow(window) != FALSE) {
        return true;
      }
    }

    std::this_thread::sleep_for(
        std::chrono::milliseconds(50));
  }

  return false;
}

bool send_f24_pulse() {
  INPUT input[2]{};

  input[0].type = INPUT_KEYBOARD;
  input[0].ki.wVk = VK_F24;

  input[1].type = INPUT_KEYBOARD;
  input[1].ki.wVk = VK_F24;
  input[1].ki.dwFlags = KEYEVENTF_KEYUP;

  return SendInput(2, input, sizeof(INPUT)) == 2;
}

}  // namespace

int wmain(int argc, wchar_t** argv) {
  const int seconds = std::clamp(
      parse_int_arg(argc, argv, L"--seconds=", 10),
      1,
      300);
  const int delay_ms = std::clamp(
      parse_int_arg(argc, argv, L"--delay-ms=", 500),
      0,
      10000);
  const int interval_ms = std::clamp(
      parse_int_arg(argc, argv, L"--interval-ms=", 50),
      10,
      1000);

  std::this_thread::sleep_for(
      std::chrono::milliseconds(delay_ms));

  if (!focus_benchmark_window()) {
    return 2;
  }

  const auto end =
      std::chrono::steady_clock::now() +
      std::chrono::seconds(seconds);

  while (std::chrono::steady_clock::now() < end) {
    if (!send_f24_pulse()) {
      return 3;
    }

    std::this_thread::sleep_for(
        std::chrono::milliseconds(interval_ms));
  }

  return 0;
}
