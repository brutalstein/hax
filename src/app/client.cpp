#include "app/client.hpp"

#include "include/cef_app.h"
#include "include/wrapper/cef_helpers.h"

namespace hax::app {

void Client::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();
  browser_ = browser;
}

bool Client::DoClose(CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();
  (void)browser;
  return false;
}

void Client::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();
  if (browser_ && browser->IsSame(browser_)) {
    browser_ = nullptr;
    CefQuitMessageLoop();
  }
}

}  // namespace hax::app
