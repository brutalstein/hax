#include "app/client.hpp"

#include "include/cef_app.h"
#include "include/wrapper/cef_helpers.h"

#include <windows.h>
#include <shellapi.h>

#include <string>

namespace hax::app {
namespace {

// App-style room bar for the official play page: paste a room link (or code)
// to join it. Only DOM chrome around the game frame is touched; the game
// itself is not modified.
constexpr char kRoomBarScript[] = R"JS(
(() => {
  const init = () => {
    if (document.getElementById('hax-app-bar')) return;
    const header = document.querySelector('.header');
    if (!header) return;

    const style = document.createElement('style');
    style.textContent = `
      .header { display: flex !important; align-items: center; }
      .header > a { display: none !important; }
      #hax-app-bar { display: flex; align-items: center; gap: 8px;
        margin: 0 12px 0 auto; flex: 0 1 640px; min-width: 0; }
      #hax-app-room { flex: 1; min-width: 0; height: 28px; box-sizing: border-box;
        padding: 0 12px; border-radius: 6px; border: 1px solid #3a4553;
        background: #11161c; color: #e9eef3; outline: none;
        font: 14px 'Open Sans', sans-serif;
        transition: border-color .15s, box-shadow .15s; }
      #hax-app-room::placeholder { color: #7f8b97; }
      #hax-app-room:focus { border-color: #4a90e2;
        box-shadow: 0 0 0 3px rgba(74, 144, 226, .25); }
      #hax-app-bar.bad #hax-app-room { border-color: #e5534b;
        box-shadow: 0 0 0 3px rgba(229, 83, 75, .2); }
      #hax-app-bar button { height: 28px; padding: 0 14px; border: 0;
        border-radius: 6px; background: #4a90e2; color: #fff; cursor: pointer;
        font: 600 13px 'Open Sans', sans-serif; white-space: nowrap; }
      #hax-app-bar button:hover { background: #3b7fd0; }
      #hax-app-msg { font: 12px 'Open Sans', sans-serif; color: #9aa6b2;
        white-space: nowrap; }
      #hax-app-msg:empty { display: none; }
      #hax-app-bar.bad #hax-app-msg { color: #ff8a80; }`;
    document.head.appendChild(style);

    const bar = document.createElement('form');
    bar.id = 'hax-app-bar';
    bar.autocomplete = 'off';
    bar.innerHTML =
      '<input id="hax-app-room" type="text" spellcheck="false" ' +
      'aria-label="Oda linki" ' +
      'placeholder="Oda linkini yapıştır, otomatik girer  (Ctrl+L)">' +
      '<button type="submit">Odaya gir</button>' +
      '<span id="hax-app-msg" role="status"></span>';
    header.appendChild(bar);

    const input = bar.querySelector('input');
    const msg = bar.querySelector('#hax-app-msg');
    const game = document.querySelector('iframe.gameframe');

    // Accepts a full/partial room link or a bare room code.
    const roomQuery = (text) => {
      text = (text || '').trim();
      if (/^[A-Za-z0-9_-]{6,}$/.test(text)) return '?c=' + text;
      let url;
      try {
        url = new URL(/^[a-z]+:\/\//i.test(text) ? text : 'https://' + text);
      } catch (e) {
        return null;
      }
      const code = url.searchParams.get('c');
      if (!/(^|\.)haxball\.com$/i.test(url.hostname) || !code ||
          !/^[A-Za-z0-9_-]+$/.test(code)) {
        return null;
      }
      return '?c=' + code + (url.searchParams.get('p') === '1' ? '&p=1' : '');
    };

    const join = (text) => {
      const query = roomQuery(text);
      if (!query) {
        bar.classList.add('bad');
        msg.textContent = 'Geçerli bir HaxBall oda linki değil';
        return;
      }
      bar.classList.remove('bad');
      msg.textContent = 'Odaya giriliyor…';
      location.href = 'https://www.haxball.com/play' + query;
    };

    bar.addEventListener('submit', (e) => {
      e.preventDefault();
      join(input.value);
    });
    input.addEventListener('paste', (e) => {
      const text = e.clipboardData.getData('text');
      if (roomQuery(text)) {
        e.preventDefault();
        input.value = text.trim();
        join(text);
      }
    });
    input.addEventListener('input', () => {
      bar.classList.remove('bad');
      msg.textContent = '';
    });
    input.addEventListener('keydown', (e) => {
      if (e.key === 'Escape') {
        input.value = '';
        input.blur();
        if (game) game.focus();
      }
    });

    // Ctrl+L focuses the bar like an address bar, also from inside the game.
    const focusBar = (e) => {
      if ((e.ctrlKey || e.metaKey) && !e.altKey && e.code === 'KeyL') {
        e.preventDefault();
        input.focus();
        input.select();
      }
    };
    window.addEventListener('keydown', focusBar, true);
    if (game) {
      const hookGame = () => {
        try {
          game.contentWindow.addEventListener('keydown', focusBar, true);
        } catch (e) {}
      };
      game.addEventListener('load', hookGame);
      hookGame();
    }
  };

  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', init);
  } else {
    init();
  }
})();
)JS";

// First launch only: a short non-blocking notice while the app samples the
// display refresh rate (~0.8 s of frames) and the WebGL renderer. The game
// stays usable underneath; the notice only warns loudly when Chromium fell
// back to software rendering, which multiplies input latency.
constexpr char kFirstRunScript[] = R"JS(
(() => {
  if (document.getElementById('hax-app-notice')) return;
  const notice = document.createElement('div');
  notice.id = 'hax-app-notice';
  notice.setAttribute('role', 'status');
  notice.style.cssText =
    'position:fixed;top:56px;left:50%;transform:translateX(-50%);' +
    'z-index:2147483647;padding:10px 18px;border-radius:8px;' +
    'background:rgba(17,22,28,.95);border:1px solid #3a4553;color:#e9eef3;' +
    'font:13px "Open Sans",sans-serif;white-space:nowrap;' +
    'box-shadow:0 6px 24px rgba(0,0,0,.45);pointer-events:none;' +
    'transition:opacity .4s';
  notice.textContent = 'İlk açılış: bu bilgisayar için ayarlanıyor…';
  document.body.appendChild(notice);

  let renderer = '';
  try {
    const gl = document.createElement('canvas').getContext('webgl');
    const info = gl && gl.getExtension('WEBGL_debug_renderer_info');
    if (info) renderer = gl.getParameter(info.UNMASKED_RENDERER_WEBGL) || '';
    gl?.getExtension('WEBGL_lose_context')?.loseContext();
  } catch (e) {}
  const software = !renderer || /swiftshader|basic render|llvmpipe/i.test(renderer);
  const gpu = (renderer.match(/^ANGLE \([^,]*,\s*(.+?)\s*(?:\(0x|Direct3D|Vulkan|OpenGL|,|\)$)/) || [])[1] || renderer;

  const deltas = [];
  let start = 0, last = 0, finished = false;
  const finish = () => {
    if (finished) return;
    finished = true;
    // Mean over the whole window: single frame timestamps are quantized.
    const total = deltas.reduce((a, b) => a + b, 0);
    const hz = deltas.length >= 10 ? Math.round(1000 * deltas.length / total) : 0;
    if (software) {
      notice.style.borderColor = '#e5534b';
      notice.textContent = 'Uyarı: GPU hızlandırma kapalı, gecikme yüksek olur. Ekran kartı sürücüsünü güncelle.';
    } else {
      notice.textContent = ['Hazır ✓', hz && hz + ' Hz', gpu, 'düşük gecikme modu']
        .filter(Boolean).join('  ·  ');
    }
    setTimeout(() => {
      notice.style.opacity = '0';
      setTimeout(() => notice.remove(), 500);
    }, software ? 9000 : 3000);
  };
  const tick = (t) => {
    if (!start) start = t;
    else deltas.push(t - last);
    last = t;
    if (t - start < 800) requestAnimationFrame(tick); else finish();
  };
  requestAnimationFrame(tick);
  setTimeout(finish, 1500);  // hidden windows get no frames
})();
)JS";

bool is_play_page(const std::string& url) {
  return url.starts_with("https://www.haxball.com/play") ||
         url.starts_with("https://haxball.com/play");
}

bool is_room_link(const std::string& url) {
  return url.starts_with("https://www.haxball.com/play?") ||
         url.starts_with("https://haxball.com/play?");
}

void bring_to_foreground(HWND window) {
  if (SetForegroundWindow(window) != FALSE) {
    return;
  }

  // Windows refuses foreground changes from processes that are not in the
  // foreground (calibration runs, or the running instance handling a second
  // launch); a held ALT key lifts that lock for this call.
  INPUT alt[2]{};
  alt[0].type = INPUT_KEYBOARD;
  alt[0].ki.wVk = VK_MENU;
  alt[1] = alt[0];
  alt[1].ki.dwFlags = KEYEVENTF_KEYUP;
  SendInput(1, &alt[0], sizeof(INPUT));
  SetForegroundWindow(window);
  SendInput(1, &alt[1], sizeof(INPUT));
}

}  // namespace

void Client::OnBeforeContextMenu(CefRefPtr<CefBrowser> browser,
                                 CefRefPtr<CefFrame> frame,
                                 CefRefPtr<CefContextMenuParams> params,
                                 CefRefPtr<CefMenuModel> model) {
  CEF_REQUIRE_UI_THREAD();
  (void)browser;
  (void)frame;

  // Keep copy/paste for text fields and selections; drop browser-only
  // entries (back, reload, print, view source) so the app feels native.
  const int text_flags = CM_TYPEFLAG_EDITABLE | CM_TYPEFLAG_SELECTION;
  if ((params->GetTypeFlags() & text_flags) == 0) {
    model->Clear();
  }
}

bool Client::OnPreKeyEvent(CefRefPtr<CefBrowser> browser,
                           const CefKeyEvent& event,
                           CefEventHandle os_event,
                           bool* is_keyboard_shortcut) {
  CEF_REQUIRE_UI_THREAD();
  (void)os_event;
  (void)is_keyboard_shortcut;

  // F11 toggles fullscreen, also while the game frame has focus. Held keys
  // auto-repeat; only the first press counts.
  if (game_ui_ && event.type == KEYEVENT_RAWKEYDOWN &&
      event.windows_key_code == VK_F11 &&
      (event.modifiers & EVENTFLAG_IS_REPEAT) == 0) {
    ToggleFullscreen(browser->GetHost()->GetWindowHandle());
    return true;
  }
  return false;
}

void Client::ToggleFullscreen(HWND window) {
  if (window == nullptr) {
    return;
  }

  // Borderless window covering the monitor: no title bar or frame, and DWM
  // can present it like exclusive fullscreen (independent flip).
  const LONG_PTR style = GetWindowLongPtrW(window, GWL_STYLE);
  if ((style & WS_OVERLAPPEDWINDOW) != 0) {
    MONITORINFO monitor{sizeof(monitor)};
    if (GetWindowPlacement(window, &windowed_placement_) &&
        GetMonitorInfoW(
            MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST),
            &monitor)) {
      SetWindowLongPtrW(window, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);
      SetWindowPos(
          window,
          HWND_TOP,
          monitor.rcMonitor.left,
          monitor.rcMonitor.top,
          monitor.rcMonitor.right - monitor.rcMonitor.left,
          monitor.rcMonitor.bottom - monitor.rcMonitor.top,
          SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
  } else {
    SetWindowLongPtrW(window, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
    SetWindowPlacement(window, &windowed_placement_);
    SetWindowPos(
        window,
        nullptr,
        0,
        0,
        0,
        0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER |
            SWP_FRAMECHANGED);
  }
}

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

    // Game window size; benchmark windows match it so calibration measures
    // the same surface the player uses.
    ShowWindow(window, SW_MAXIMIZE);

    if (!game_ui_) {
      // Benchmark windows are started from a background script and may not
      // get the foreground; an occluded Chromium window stops presenting,
      // which leaves PresentMon with nothing to measure.
      SetWindowPos(
          window,
          HWND_TOPMOST,
          0,
          0,
          0,
          0,
          SWP_NOMOVE | SWP_NOSIZE);
    }
    bring_to_foreground(window);
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

bool Client::OnBeforePopup(CefRefPtr<CefBrowser> browser,
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
                           bool* no_javascript_access) {
  CEF_REQUIRE_UI_THREAD();
  (void)frame;
  (void)popup_id;
  (void)target_frame_name;
  (void)target_disposition;
  (void)popupFeatures;
  (void)windowInfo;
  (void)client;
  (void)settings;
  (void)extra_info;
  (void)no_javascript_access;

  // Never open extra browser windows: room links stay in the app, other
  // user-clicked links go to the default browser.
  const std::string url = target_url.ToString();
  if (game_ui_ && is_room_link(url)) {
    browser->GetMainFrame()->LoadURL(target_url);
  } else if (game_ui_ && user_gesture &&
             (url.starts_with("https://") || url.starts_with("http://"))) {
    ShellExecuteW(
        nullptr,
        L"open",
        target_url.ToWString().c_str(),
        nullptr,
        nullptr,
        SW_SHOWNORMAL);
  }
  return true;
}

void Client::OnLoadStart(CefRefPtr<CefBrowser> browser,
                         CefRefPtr<CefFrame> frame,
                         TransitionType transition_type) {
  (void)browser;
  (void)transition_type;
  InjectRoomBar(frame);
}

void Client::OnLoadEnd(CefRefPtr<CefBrowser> browser,
                       CefRefPtr<CefFrame> frame,
                       int httpStatusCode) {
  (void)browser;
  (void)httpStatusCode;
  InjectRoomBar(frame);

  if (first_run_notice_pending_ && frame->IsMain() &&
      is_play_page(frame->GetURL())) {
    first_run_notice_pending_ = false;
    frame->ExecuteJavaScript(kFirstRunScript, frame->GetURL(), 0);
  }
}

void Client::InjectRoomBar(CefRefPtr<CefFrame> frame) {
  // The script is idempotent: load start shows the bar as soon as the DOM is
  // ready, load end covers anything start missed.
  if (game_ui_ && frame->IsMain() && is_play_page(frame->GetURL())) {
    frame->ExecuteJavaScript(kRoomBarScript, frame->GetURL(), 0);
  }
}

void Client::BringToFront() {
  CEF_REQUIRE_UI_THREAD();
  if (!browser_) {
    return;
  }

  HWND window = browser_->GetHost()->GetWindowHandle();
  if (window == nullptr) {
    return;
  }

  if (IsIconic(window)) {
    ShowWindow(window, SW_RESTORE);
  }
  bring_to_foreground(window);
}

}  // namespace hax::app
