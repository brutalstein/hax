#pragma once

#include "include/cef_client.h"

#include <windows.h>

namespace hax::app {

class Client final : public CefClient,
                     public CefContextMenuHandler,
                     public CefKeyboardHandler,
                     public CefLifeSpanHandler,
                     public CefLoadHandler {
 public:
  Client(bool game_ui, bool first_run)
      : game_ui_(game_ui), first_run_notice_pending_(game_ui && first_run) {}

  CefRefPtr<CefContextMenuHandler> GetContextMenuHandler() override {
    return this;
  }

  CefRefPtr<CefKeyboardHandler> GetKeyboardHandler() override {
    return this;
  }

  CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override {
    return this;
  }

  CefRefPtr<CefLoadHandler> GetLoadHandler() override {
    return this;
  }

  void OnBeforeContextMenu(CefRefPtr<CefBrowser> browser,
                           CefRefPtr<CefFrame> frame,
                           CefRefPtr<CefContextMenuParams> params,
                           CefRefPtr<CefMenuModel> model) override;

  bool OnPreKeyEvent(CefRefPtr<CefBrowser> browser,
                     const CefKeyEvent& event,
                     CefEventHandle os_event,
                     bool* is_keyboard_shortcut) override;

  void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;
  bool DoClose(CefRefPtr<CefBrowser> browser) override;
  void OnBeforeClose(CefRefPtr<CefBrowser> browser) override;

  bool OnBeforePopup(CefRefPtr<CefBrowser> browser,
                     CefRefPtr<CefFrame> frame,
                     int popup_id,
                     const CefString& target_url,
                     const CefString& target_frame_name,
                     WindowOpenDisposition target_disposition,
                     bool user_gesture,
                     const CefPopupFeatures& popupFeatures,
                     CefWindowInfo& windowInfo,
                     CefRefPtr<CefClient>& client,
                     CefBrowserSettings& settings,
                     CefRefPtr<CefDictionaryValue>& extra_info,
                     bool* no_javascript_access) override;

  void OnLoadStart(CefRefPtr<CefBrowser> browser,
                   CefRefPtr<CefFrame> frame,
                   TransitionType transition_type) override;

  void OnLoadEnd(CefRefPtr<CefBrowser> browser,
                 CefRefPtr<CefFrame> frame,
                 int httpStatusCode) override;

  void BringToFront();

 private:
  void InjectRoomBar(CefRefPtr<CefFrame> frame);
  void ToggleFullscreen(HWND window);

  bool game_ui_;
  bool first_run_notice_pending_;
  WINDOWPLACEMENT windowed_placement_{sizeof(WINDOWPLACEMENT)};
  CefRefPtr<CefBrowser> browser_;

  IMPLEMENT_REFCOUNTING(Client);
  DISALLOW_COPY_AND_ASSIGN(Client);
};

}  // namespace hax::app
