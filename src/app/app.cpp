#include "app/app.hpp"
#include "app/client.hpp"

#include "include/cef_browser.h"
#include "include/wrapper/cef_helpers.h"

#include <filesystem>
#include <string>
#include <utility>

namespace hax::app {
namespace {

// Low-latency canvas: HaxBall's canvases are created with
// desynchronized: true, so Chromium may present them straight to the screen
// (front buffer / overlay) instead of waiting for the compositor, saving up
// to a frame of input-to-photon latency. Possible cost: tearing. The game code
// itself is untouched; only the context creation option is added.
constexpr char kLowLatencyCanvasScript[] = R"JS(
(() => {
  const getContext = HTMLCanvasElement.prototype.getContext;
  HTMLCanvasElement.prototype.getContext = function (type, attributes) {
    if (type === '2d' || type === 'webgl' || type === 'webgl2') {
      attributes = Object.assign({}, attributes, { desynchronized: true });
    }
    return getContext.call(this, type, attributes);
  };
})();
)JS";

bool is_haxball_url(const std::string& url) {
  return url.starts_with("https://www.haxball.com/") ||
         url.starts_with("https://haxball.com/");
}

}  // namespace

App::App(LaunchOptions options) : options_(std::move(options)) {}

void App::OnContextCreated(CefRefPtr<CefBrowser> browser,
                           CefRefPtr<CefFrame> frame,
                           CefRefPtr<CefV8Context> context) {
  (void)browser;

  // Runs in the renderer before any page script of the frame, so the game
  // frame's canvas is already created with the low-latency option.
  if (!is_haxball_url(frame->GetURL().ToString())) {
    return;
  }

  CefRefPtr<CefV8Value> result;
  CefRefPtr<CefV8Exception> exception;
  context->Eval(kLowLatencyCanvasScript, frame->GetURL(), 0, result, exception);
}

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
  window_info.SetAsPopup(nullptr, L"Haxball App");
  // Alloy style: a plain app window without Chrome tabs, toolbar or session
  // restore. The title also stays "Haxball App", which the calibration input
  // helper relies on to find the benchmark window.
  window_info.runtime_style = CEF_RUNTIME_STYLE_ALLOY;

  CefBrowserSettings browser_settings;
  client_ = new Client(!options_.benchmark_mode, options_.first_run);

  std::string target_url = options_.url;
  if (options_.benchmark_mode) {
    const std::filesystem::path path(options_.benchmark_path);
    target_url = "file:///" + path.generic_string();
  }

  CefBrowserHost::CreateBrowser(
      window_info,
      client_,
      target_url,
      browser_settings,
      nullptr,
      nullptr);
}

bool App::OnAlreadyRunningAppRelaunch(
    CefRefPtr<CefCommandLine> command_line,
    const CefString& current_directory) {
  CEF_REQUIRE_UI_THREAD();
  (void)command_line;
  (void)current_directory;

  // A second launch focuses the running game instead of opening another
  // (Chrome-style) window.
  if (client_) {
    client_->BringToFront();
  }
  return true;
}

}  // namespace hax::app
