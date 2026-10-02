# Research basis

Foundation decisions were checked against primary/current sources on 2026-10-02.

## HaxBall

The official input-lag update documents Chromium desynchronized canvas and direct processing of player input in HTML input events:

https://blog.haxball.com/2023/05/01/input-lag-improvments.html

The HTML5 announcement documents WebRTC netplay and immediate network-event handling:

https://blog.haxball.com/2017/10/25/html5-haxball-is-here.html

## CEF

Pinned stable build at project creation:

CEF 154.0.28+g564dd6c+chromium-154.0.8037.58
published 2026-09-25.

Official build index:

https://cef-builds.spotifycdn.com/index.html

CEF exposes OnBeforeCommandLineProcessing for controlled Chromium switches.

## Windows CPU and QoS

CPU Set topology:

https://learn.microsoft.com/windows/win32/procthread/getsystemcpusetinformation

Process CPU Sets:

https://learn.microsoft.com/windows/win32/api/processthreadsapi/nf-processthreadsapi-setprocessdefaultcpusets

Process power throttling:

https://learn.microsoft.com/windows/win32/api/processthreadsapi/nf-processthreadsapi-setprocessinformation

## Presentation measurement

PresentMon capture metrics:

https://github.com/GameTechDev/PresentMon/blob/main/README-CaptureApplication.md

## Existing public baseline

The public oghb/haxball-client is Electron-based and supports unlockable FPS; its README notes unlimited FPS may not work flawlessly for everyone:

https://github.com/oghb/haxball-client
