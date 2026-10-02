#pragma once

#include "app/client.hpp"
#include "app/launch_options.hpp"
#include "include/cef_app.h"

namespace hax::app {

class App final : public CefApp,
                  public CefBrowserProcessHandler,
                  public CefRenderProcessHandler {
 public:
  explicit App(LaunchOptions options);

  CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override {
    return this;
  }

  CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override {
    return this;
  }

  void OnContextCreated(CefRefPtr<CefBrowser> browser,
                        CefRefPtr<CefFrame> frame,
                        CefRefPtr<CefV8Context> context) override;

  void OnBeforeCommandLineProcessing(
      const CefString& process_type,
      CefRefPtr<CefCommandLine> command_line) override;

  void OnBeforeChildProcessLaunch(
      CefRefPtr<CefCommandLine> command_line) override;

  void OnContextInitialized() override;

  bool OnAlreadyRunningAppRelaunch(
      CefRefPtr<CefCommandLine> command_line,
      const CefString& current_directory) override;

 private:
  LaunchOptions options_;
  CefRefPtr<Client> client_;

  IMPLEMENT_REFCOUNTING(App);
  DISALLOW_COPY_AND_ASSIGN(App);
};

}  // namespace hax::app
