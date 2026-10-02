#pragma once

#include "app/launch_options.hpp"
#include "include/cef_app.h"

namespace hax::app {

class App final : public CefApp, public CefBrowserProcessHandler {
 public:
  explicit App(LaunchOptions options);

  CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override {
    return this;
  }

  void OnBeforeCommandLineProcessing(
      const CefString& process_type,
      CefRefPtr<CefCommandLine> command_line) override;

  void OnBeforeChildProcessLaunch(
      CefRefPtr<CefCommandLine> command_line) override;

  void OnContextInitialized() override;

 private:
  LaunchOptions options_;

  IMPLEMENT_REFCOUNTING(App);
  DISALLOW_COPY_AND_ASSIGN(App);
};

}  // namespace hax::app
