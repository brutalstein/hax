# Architecture

## Boundary

Hax is a runtime optimizer around the official HaxBall Chromium application. The browser page remains authoritative for game logic, networking, input handling, and rendering content.

The latency path stays short:

```text
HID -> Windows -> Chromium input event -> HaxBall -> canvas -> Chromium compositor -> present -> display
```

The project intentionally does not create a second Raw Input -> IPC -> JavaScript gameplay-input route. HaxBall already processes player input directly in HTML input events and enables desynchronized canvas rendering on Chromium.

## Modules

### hax_core

Pure C++20 and platform-independent:

- candidate generation;
- robust timing statistics;
- feasibility constraints;
- scoring;
- best-candidate selection.

It can be compiled and tested without CEF or Windows.

### hax_platform

Windows-only adapters:

- GetSystemCpuSetInformation;
- DXGI 1.6 GPU enumeration;
- current display mode;
- AC/DC power state;
- process priority;
- process default CPU Sets;
- process power-throttling policy.

All mutations are process-local. No registry, BCD, service, or system-wide timer changes are made.

### hax

CEF host:

- normal windowed rendering;
- official HaxBall URL;
- Chromium command-line policy;
- same executable for Chromium subprocesses;
- synthetic desynchronized-canvas calibration page.

## CEF instead of Electron

Electron can unlock Chromium frame rate, and public HaxBall clients already do so. CEF removes the Node/Electron product layer and gives the native host a smaller surface for Windows scheduling, hardware probing, and reproducible Chromium version control.

This is not a claim that CEF is inherently faster. Performance advantage must come from measured configuration choices.

## GPU policy

System-default GPU is always a candidate. Multi-GPU machines can additionally test low-power and high-performance Chromium preferences. A dGPU is not automatically selected because hybrid laptops can pay a cross-adapter/display-path cost.

## CPU policy

Windows default scheduling remains a candidate. Heterogeneous processors may additionally test the highest EfficiencyClass CPU Sets. The policy is retained only if calibration wins.

## Calibration-only input measurement

The benchmark path has a small Win32 input-pulse helper that sends F24 keyboard events at a fixed interval. The local benchmark page responds with a large canvas transition so PresentMon can observe keyboard-input-to-displayed-frame timing. The helper is not loaded or used during HaxBall gameplay and does not inject movement or game controls.

## Telemetry

Heavy ETW/PresentMon capture is for calibration and regression testing. Gameplay mode keeps this measurement stack out of the gameplay path to reduce observer effects.

## Security release gate

The remote HaxBall page receives no Node.js bridge or privileged native JavaScript API.

The current application uses unsandboxed CEF mode for a straightforward deterministic build. A signed public release should migrate to the current CEF Windows bootstrap/sandbox model before distribution. This is an explicit release gate.
