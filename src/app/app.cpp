#include "app/app.hpp"
#include "app/client.hpp"

#include "include/cef_browser.h"
#include "include/wrapper/cef_helpers.h"

#include <filesystem>
#include <utility>

namespace hax::app {

App::App(LaunchOptions options) : options_(std::move(options)) {}

void App::OnBeforeCommandLineProcessing(
    const CefString& process_type,
    CefRefPtr<CefCommandLine> command_line) {
  if (!process_type.empty()) {
    return;
  }

  if (options_.profile.disable_vsync) {
    command_line->AppendSwitch("disable-gpu-vsync");
  }

  if (options_.profile.frame == hax::core::FramePolicy::uncapped) {
    command_line->AppendSwitch("disable-frame-rate-limit");
  }

  switch (options_.profile.gpu) {
    case hax::core::GpuPreference::high_performance:
      command_line->AppendSwitch("force-high-performance-gpu");
      break;
    case hax::core::GpuPreference::low_power:
      command_line->AppendSwitchWithValue(
          "gpu-switching", "force_integrated");
      break;
    case hax::core::GpuPreference::system_default:
      break;
  }

  command_line->AppendSwitch("disable-renderer-backgrounding");
  command_line->AppendSwitch("disable-background-timer-throttling");
}

void App::OnBeforeChildProcessLaunch(
    CefRefPtr<CefCommandLine> command_line) {
  switch (options_.profile.frame) {
    case hax::core::FramePolicy::browser_default:
      command_line->AppendSwitchWithValue("hax-fps", "default");
      break;
    case hax::core::FramePolicy::uncapped:
      command_line->AppendSwitchWithValue("hax-fps", "uncapped");
      break;
  }

  switch (options_.profile.gpu) {
    case hax::core::GpuPreference::system_default:
      command_line->AppendSwitchWithValue("hax-gpu", "default");
      break;
    case hax::core::GpuPreference::low_power:
      command_line->AppendSwitchWithValue("hax-gpu", "low");
      break;
    case hax::core::GpuPreference::high_performance:
      command_line->AppendSwitchWithValue("hax-gpu", "high");
      break;
  }

  switch (options_.profile.cpu) {
    case hax::core::CpuPolicy::system_default:
      command_line->AppendSwitchWithValue("hax-cpu", "default");
      break;
    case hax::core::CpuPolicy::prefer_performance_cores:
      command_line->AppendSwitchWithValue("hax-cpu", "performance");
      break;
  }

  switch (options_.profile.priority) {
    case hax::core::PriorityPolicy::normal:
      command_line->AppendSwitchWithValue("hax-priority", "normal");
      break;
    case hax::core::PriorityPolicy::above_normal:
      command_line->AppendSwitchWithValue("hax-priority", "above");
      break;
    case hax::core::PriorityPolicy::high:
      command_line->AppendSwitchWithValue("hax-priority", "high");
      break;
  }

  command_line->AppendSwitch("hax-no-profile");
}

void App::OnContextInitialized() {
  CEF_REQUIRE_UI_THREAD();

  CefWindowInfo window_info;
  window_info.SetAsPopup(nullptr, L"Hax Performance Runtime");

  CefBrowserSettings browser_settings;
  CefRefPtr<Client> client(new Client());

  std::string target_url = options_.url;
  if (options_.benchmark_mode) {
    const std::filesystem::path path(options_.benchmark_path);
    target_url = "file:///" + path.generic_string();
  }

  CefBrowserHost::CreateBrowser(
      window_info,
      client,
      target_url,
      browser_settings,
      nullptr,
      nullptr);
}

}  // namespace hax::app
