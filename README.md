# Hax Performance Runtime

[![Core CI](https://github.com/brutalstein/hax/actions/workflows/core-ci.yml/badge.svg)](https://github.com/brutalstein/hax/actions/workflows/core-ci.yml)
[![Windows CEF App](https://github.com/brutalstein/hax/actions/workflows/windows-app.yml/badge.svg)](https://github.com/brutalstein/hax/actions/workflows/windows-app.yml)

An **unofficial, Windows-first native performance runtime** for the official HaxBall web client.

Hax does not reimplement, patch, decompile, or reverse-engineer HaxBall. It loads the official `https://www.haxball.com/play` page in a pinned Chromium Embedded Framework (CEF) runtime and optimizes the environment around it.

## Goal

The target is **minimum stable input-to-display latency**, not the largest FPS counter.

A configuration is accepted only when measured data beats the browser-default baseline while remaining stable across tail latency, frame-time jitter, sustained drift, and a repeated-baseline consistency check.

~~~text
HID
 └─> Windows
     └─> Chromium input event
         └─> official HaxBall
             └─> desynchronized canvas
                 └─> Chromium compositor
                     └─> display
~~~

There is no custom Raw Input -> IPC -> JavaScript gameplay path.

## Architecture

~~~text
Win32 / C++20
├─ hax_core
│  ├─ robust statistics
│  ├─ hardware-pruned exhaustive candidate generation
│  └─ constrained multi-objective optimizer
├─ hax_platform
│  ├─ Windows CPU-set topology
│  ├─ DXGI GPU inventory
│  ├─ display / AC-power probe
│  └─ reversible process-local scheduling/QoS policy
├─ hax
│  ├─ pinned CEF / Chromium
│  ├─ official HaxBall URL
│  └─ normal windowed GPU rendering
└─ calibration
   ├─ desynchronized-canvas workload
   ├─ PresentMon capture
   ├─ process + swapchain stream isolation
   └─ persisted per-machine winner
~~~

See `docs/ARCHITECTURE.md` and `docs/ALGORITHM_ANALYSIS.md`.

## Windows package

The **Windows CEF App** workflow produces the downloadable artifact:

~~~text
HaxPerformanceRuntime-win64
~~~

It contains the native runtime, CEF payload, calibration tools, scripts, notices, and documentation.

## First run

From the extracted package:

~~~powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run.ps1
~~~

When no machine profile exists, the launcher runs one-time hardware-adaptive calibration and then launches HaxBall.

PresentMon 2.6.0 x64 is downloaded from its official GitHub release only when required and is verified against the pinned SHA-256 before execution.

Recalibrate after a major GPU driver, Windows, monitor, or hardware change:

~~~powershell
.\scripts\run.ps1 -Recalibrate
~~~

Diagnostic launch without calibration:

~~~powershell
.\scripts\run.ps1 -SkipCalibration
~~~

The selected machine profile is stored at:

~~~text
%LOCALAPPDATA%\HaxPerformanceRuntime\profile.ini
~~~

## Search space

Calibration exhaustively tests the hardware-relevant configuration space after pruning impossible or irrelevant branches:

- frame policy: browser default / uncapped;
- GPU policy: system default / integrated / high-performance when multiple hardware adapters exist;
- CPU policy: Windows scheduler / highest-EfficiencyClass CPU Sets on heterogeneous CPUs;
- process priority: normal / above-normal / high.

Maximum size is **36 candidates + one repeated baseline**. A single-GPU homogeneous desktop tests only 6 + baseline repeat.

The optimizer evaluates median and p99 latency, p99 frame time, MAD frame-time jitter, present-to-display timing, uncertainty, and sustained first-vs-last frame-time drift. The complete calibration is rejected if its repeated baseline changes by more than 8%.

## Reproducible dependencies

CEF is pinned to:

~~~text
154.0.28+g564dd6c+chromium-154.0.8037.58
~~~

The CEF archive is hash-verified during acquisition.

Calibration tooling is pinned to:

~~~text
PresentMon 2.6.0 x64
SHA-256:
b2a706bc6ad475749e3b7e3409263aa1e6906d45bdcf993f6dbc0f660188f1af
~~~

## Developer build

Requirements:

- Windows 10/11 x64;
- Visual Studio 2022 with Desktop development with C++;
- CMake 3.24+.

~~~powershell
.\scripts\build.ps1 -Configuration Release -App
.\scripts\run.ps1
~~~

Or use CMake presets:

~~~powershell
cmake --preset windows-app
cmake --build --preset windows-release
ctest --preset windows
~~~

Portable core:

~~~powershell
cmake --preset core
cmake --build --preset core-release
ctest --preset core
~~~

## Experimental switches

~~~text
--hax-fps=default|uncapped
--hax-gpu=default|low|high
--hax-cpu=default|performance
--hax-priority=normal|above|high
--hax-benchmark
--hax-no-profile
~~~

## Deliberate non-features

- no HaxBall source modification;
- no gameplay-input reinjection;
- no REALTIME priority;
- no global HPET/BCD/registry gaming tweaks;
- no assumption that the discrete GPU is always best;
- no off-screen CEF rendering;
- no permanent OS changes;
- no generic latency claims without hardware evidence.

Hardware-specific performance claims require captured measurements on that hardware. See `docs/BENCHMARKING.md`.

## Validation

The portable core is continuously built and tested on Linux and Windows. The native Windows CEF runtime is compiled and linked in CI against the pinned CEF distribution, and the workflow packages a self-contained Windows artifact.

The current 36-candidate optimizer micro-benchmark and validation boundary are recorded in `docs/VALIDATION.md`.

## Security

The official HaxBall page receives no privileged native JavaScript bridge.

Current development builds still use CEF's sandbox-disabled executable mode. Migration to CEF's current Windows bootstrap/sandbox packaging is explicitly tracked as a release-hardening gate. See `SECURITY.md`.
