<p align="center">
  <img src="assets/windows/haxball_app_source.png" width="128" alt="Haxball App icon">
</p>

# Haxball App

[![Core CI](https://github.com/brutalstein/hax/actions/workflows/core-ci.yml/badge.svg)](https://github.com/brutalstein/hax/actions/workflows/core-ci.yml)
[![Windows App](https://github.com/brutalstein/hax/actions/workflows/windows-app.yml/badge.svg)](https://github.com/brutalstein/hax/actions/workflows/windows-app.yml)

## Download

**[Download Haxball App for Windows x64](https://github.com/brutalstein/hax/releases/latest/download/Haxball-App-Windows-x64.zip)**

Haxball App is an unofficial Windows-first native performance client for the official HaxBall web game. It loads the official HaxBall site inside a pinned Chromium Embedded Framework runtime and calibrates the local machine for minimum stable latency rather than simply chasing the largest FPS counter.

## Install

1. Download the ZIP above.
2. Extract it.
3. Double-click **Install Haxball App.cmd**.

The installer copies the application to:

```text
%LOCALAPPDATA%\Programs\Haxball App
```

It creates **Haxball App** shortcuts on the Desktop and Start Menu. The supplied H icon is embedded directly in **Haxball App.exe** and is also assigned to the native CEF window, so Explorer, the desktop shortcut and the Windows taskbar use the same application icon.

On first launch, Haxball App performs hardware-adaptive calibration and stores the measured winning profile before opening the official HaxBall page.

## What it optimizes

The calibration engine can evaluate up to 36 hardware-relevant configurations across frame policy, GPU selection, CPU Sets and process priority. Selection uses robust timing evidence including p50/p99 latency, p99 frame time, MAD jitter, PresentMon display timing, CPU/GPU utilization, hybrid/cross-adapter presentation and sustained frame-time drift.

Heavy PresentMon/ETW capture is used only during calibration and is kept out of normal gameplay.

The selected machine profile is stored at:

```text
%LOCALAPPDATA%\HaxballApp\profile.ini
```

A hardware/runtime fingerprint invalidates stale calibration when relevant hardware, display, driver, Windows or CEF context changes.

## Performance boundaries

- no HaxBall source modification or reverse engineering;
- no replacement gameplay-input path;
- no REALTIME priority;
- no global HPET/BCD/registry gaming tweaks;
- no assumption that the discrete GPU is always lower latency;
- no off-screen CEF rendering;
- no permanent OS tuning changes.

## Developer build

```powershell
.\scripts\build.ps1 -Configuration Release -App
.\scripts\run.ps1
```

Or:

```powershell
cmake --preset windows-app
cmake --build --preset windows-release
ctest --preset windows
```

See `docs/ARCHITECTURE.md`, `docs/ALGORITHM_ANALYSIS.md`, `docs/BENCHMARKING.md` and `docs/VALIDATION.md` for the implementation and evidence model.

## Engine and measurement dependencies

- CEF 154.0.28 + Chromium 154.0.8037.58
- PresentMon 2.6.0 x64, SHA-256 verified before calibration use

## Security

The official remote HaxBall page receives no privileged native JavaScript bridge. The current development build still uses CEF's sandbox-disabled executable mode; see `SECURITY.md` for the explicit public-release hardening boundary.
