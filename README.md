# Hax Performance Runtime

An **unofficial**, Windows-first native performance runtime for the official HaxBall web client.

The project does not reimplement, patch, decompile, or reverse-engineer HaxBall. It embeds the official `https://www.haxball.com/play` page in Chromium Embedded Framework (CEF) and optimizes the environment around it.

## Design goal

The objective is **minimum stable input-to-display latency**, not the largest FPS counter.

A configuration is accepted only when measured data shows that it improves latency while respecting stability and thermal constraints. Raw FPS is treated as an experimental variable, not as the objective function.

## Architecture

```text
Win32 / C++20
├─ hax_core
│  ├─ robust statistics
│  ├─ bounded candidate generation
│  └─ multi-objective latency optimizer
├─ hax_platform (Windows)
│  ├─ CPU-set topology probe
│  ├─ GPU enumeration
│  ├─ display / power-state probe
│  └─ reversible runtime policy
└─ hax (CEF)
   ├─ official HaxBall URL
   ├─ windowed GPU rendering
   ├─ current Chromium runtime
   └─ calibration flags
```

See `docs/ARCHITECTURE.md` and `docs/ALGORITHM_ANALYSIS.md`.

## Reproducible engine

The app build pins CEF **154.0.28+g564dd6c+chromium-154.0.8037.58** (current stable CEF build dated 2026-09-25 when this foundation was created). The archive SHA-1 is fetched from the official CEF automated build service and verified by CMake before extraction.

## Build

### Portable core only

```powershell
cmake -S . -B build -DHAX_BUILD_APP=OFF
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

### Windows application

Requirements:

- Windows 10/11 x64
- Visual Studio 2022 with Desktop development with C++
- CMake 3.24+
- Internet access on the first build for the pinned CEF distribution (~350 MB)

```powershell
.\scripts\build.ps1 -Configuration Release -App
```

Run:

```powershell
.\build\Release\hax.exe
```

Useful experimental switches:

```text
--hax-fps=default
--hax-fps=uncapped
--hax-gpu=high
--hax-gpu=low
--hax-cpu=performance
--hax-priority=normal
--hax-priority=high
--hax-benchmark
```

## Calibration

For full frame-presentation measurements, install Intel PresentMon and run:

```powershell
.\scripts\calibrate.ps1
```

The script launches the native synthetic desynchronized-canvas workload under multiple process/GPU/CPU policies and saves separate PresentMon CSV captures. Do not run heavy ETW telemetry continuously during gameplay; calibration is intentionally an offline phase.

## What is deliberately not done

- no HaxBall code modification
- no gameplay input reinjection
- no `REALTIME_PRIORITY_CLASS`
- no global HPET/BCD/registry "gaming tweaks"
- no forced dGPU assumption
- no off-screen CEF rendering
- no permanent OS changes

Every optimization is process-local, reversible, and intended to be selected from measurements.

## Status

The repository contains the first complete native foundation:

- portable optimizer and statistics core
- Windows CPU/GPU/display/power probing
- process-local scheduling/power policy
- CEF application loading official HaxBall
- deterministic CEF dependency pin
- local render calibration workload
- PresentMon calibration harness
- unit tests, micro-benchmark, and cross-platform CI

Hardware-specific performance claims must be made from captured measurements on that hardware. See `docs/BENCHMARKING.md`.
