#include "app/client.hpp"

#include "include/cef_app.h"
#include "include/wrapper/cef_helpers.h"

#include <windows.h>

namespace hax::app {

void Client::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();
  browser_ = browser;

  HWND window = browser->GetHost()->GetWindowHandle();
  if (window != nullptr) {
    HINSTANCE instance = GetModuleHandleW(nullptr);

    HICON large_icon = static_cast<HICON>(
        LoadImageW(
            instance,
            MAKEINTRESOURCEW(101),
            IMAGE_ICON,
            GetSystemMetrics(SM_CXICON),
            GetSystemMetrics(SM_CYICON),
            LR_DEFAULTCOLOR));

    HICON small_icon = static_cast<HICON>(
        LoadImageW(
            instance,
            MAKEINTRESOURCEW(101),
            IMAGE_ICON,
            GetSystemMetrics(SM_CXSMICON),
            GetSystemMetrics(SM_CYSMICON),
            LR_DEFAULTCOLOR));

    if (large_icon != nullptr) {
      SendMessageW(
          window,
          WM_SETICON,
          ICON_BIG,
          reinterpret_cast<LPARAM>(large_icon));
    }

    if (small_icon != nullptr) {
      SendMessageW(
          window,
          WM_SETICON,
          ICON_SMALL,
          reinterpret_cast<LPARAM>(small_icon));
    }
  }
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
