<p align="center">
  <img src="assets/windows/haxball_app_source.png" width="128" alt="Haxball App icon">
</p>

# Haxball App

[![Core CI](https://github.com/brutalstein/hax/actions/workflows/core-ci.yml/badge.svg)](https://github.com/brutalstein/hax/actions/workflows/core-ci.yml)
[![Windows App](https://github.com/brutalstein/hax/actions/workflows/windows-app.yml/badge.svg)](https://github.com/brutalstein/hax/actions/workflows/windows-app.yml)

## Download

**[Download Haxball App for Windows x64](https://github.com/brutalstein/hax/releases/latest/download/Haxball-App-Windows-x64.zip)**

Haxball App is an unofficial Windows-first native performance client for the official HaxBall web game. It loads the official HaxBall site inside a pinned Chromium Embedded Framework runtime and calibrates the local machine for minimum stable latency rather than simply chasing the largest FPS counter.

## Run

1. Download the ZIP above.
2. Extract it.
3. Double-click **Haxball App.exe**.

No installer is required. The app opens as a single plain window (no browser tabs or address bar), maximized, on the official HaxBall play page.

- **First launch:** the game opens immediately. A short notice ("İlk açılış: bu bilgisayar için ayarlanıyor…") checks the display refresh rate and GPU for about a second, stores the low-latency profile and fades out. It warns if GPU acceleration is unavailable.
- **Every later launch:** the game opens instantly with the stored profile; nothing runs in the background.
- **Joining by link:** paste a room link (or room code) into the bar at the top of the page; it joins immediately. `Ctrl+L` focuses the bar, `Esc` returns to the game. Room links clicked inside the game stay in the app; other links open in your default browser.
- **Fullscreen:** `F11` toggles borderless fullscreen (works while the game has focus).
- **Low-latency canvas:** HaxBall's canvases are created with `desynchronized: true`, letting Chromium present the game directly instead of waiting for the compositor (possible tearing). The game code itself is not modified.
- **Launching again** while the game is open just brings the existing window to the front.

The stored profile comes from full PresentMon calibration: browser vsync pacing gave the lowest frame-to-display latency (about 9.5 ms versus 21 ms uncapped at 300 Hz), while GPU selection and CPU policy were within measurement noise.

**Recalibrate Haxball App.cmd** is optional. Close the game and run it to measure your own PC (benchmark windows open and close for a few minutes, then the game opens). If it fails, the current profile stays and the diagnostic is written to:

```text
%LOCALAPPDATA%\HaxballApp\last-calibration-error.txt
```

**Install Haxball App.cmd** is optional. Use it only if you want the app copied to a permanent location with Desktop and Start Menu shortcuts.

The optional installer copies the application to:

```text
%LOCALAPPDATA%\Programs\Haxball App
```

It creates **Haxball App** shortcuts on the Desktop and Start Menu that point directly to **Haxball App.exe**. The supplied H icon is embedded directly in **Haxball App.exe** and is also assigned to the native CEF window, so Explorer, the desktop shortcut and the Windows taskbar use the same application icon.

## What it optimizes

The calibration engine can evaluate up to 36 hardware-relevant configurations across frame policy, GPU selection, CPU Sets and process priority. Selection uses robust timing evidence including p50/p99 latency, p99 frame time, MAD jitter, PresentMon display timing, CPU/GPU utilization, hybrid/cross-adapter presentation and sustained frame-time drift.

Heavy PresentMon/ETW capture is used only during calibration and is kept out of normal gameplay.

The selected machine profile is stored at:

```text
%LOCALAPPDATA%\HaxballApp\profile.ini
```

Delete it to return to the default profile on the next launch.

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
.\build\Release\"Haxball App.exe"
.\scripts\run.ps1   # optional full calibration
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
