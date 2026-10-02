# Research basis

Foundation decisions were checked against primary/current sources on 2026-10-02.

## HaxBall

Official input-lag update documenting Chromium desynchronized canvas and direct player-input processing in HTML input events:

https://blog.haxball.com/2023/05/01/input-lag-improvments.html

HTML5/WebRTC announcement:

https://blog.haxball.com/2017/10/25/html5-haxball-is-here.html

## CEF

Pinned build:

~~~text
CEF 154.0.28+g564dd6c+chromium-154.0.8037.58
~~~

Official automated build index:

https://cef-builds.spotifycdn.com/index.html

CEF provides OnBeforeCommandLineProcessing for controlled Chromium switches and OnBeforeChildProcessLaunch for subprocess policy propagation.

Current CEF Windows distributions use Visual Studio 2022 as the supported/tested toolchain for standard binary builds.

## Chromium GPU behavior

Current Chromium source defines the Windows force-high-performance-gpu switch and maps it into the GPU process' high-performance GPU workaround:

https://chromium.googlesource.com/chromium/src/+/master/gpu/config/gpu_switches.cc

https://chromium.googlesource.com/chromium/src/+/master/content/browser/gpu/gpu_process_host.cc

Current GL switches also expose gpu-switching with force_integrated / force_discrete values:

https://chromium.googlesource.com/chromium/+/HEAD/ui/gl/gl_switches.cc

The project therefore does not use the nonexistent force-low-power-gpu flag. Its integrated-GPU experiment uses gpu-switching=force_integrated, while the high-performance experiment uses force-high-performance-gpu.

## Windows CPU and QoS

CPU Set topology:

https://learn.microsoft.com/windows/win32/procthread/getsystemcpusetinformation

Process CPU Sets:

https://learn.microsoft.com/windows/win32/api/processthreadsapi/nf-processthreadsapi-setprocessdefaultcpusets

Process power throttling:

https://learn.microsoft.com/windows/win32/api/processthreadsapi/nf-processthreadsapi-setprocessinformation

## Presentation measurement

PresentMon console/capture documentation:

https://github.com/GameTechDev/PresentMon/blob/main/README-ConsoleApplication.md

https://github.com/GameTechDev/PresentMon/blob/main/README-CaptureApplication.md

Calibration pins the official PresentMon 2.6.0 x64 console binary released 2026-09-21 and verifies this SHA-256 before execution:

~~~text
b2a706bc6ad475749e3b7e3409263aa1e6906d45bdcf993f6dbc0f660188f1af
~~~

## Existing public baseline

The public oghb/haxball-client is Electron-based and supports unlockable FPS; its README notes that unlimited FPS may not work flawlessly for everyone:

https://github.com/oghb/haxball-client
