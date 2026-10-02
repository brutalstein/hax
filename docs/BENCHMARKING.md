# Benchmarking and evidence

## Evidence hierarchy

1. On-device input/presentation measurements with PresentMon or equivalent ETW capture.
2. Repeatable A/B captures with the same scene, resolution, power state, thermal state, driver and display.
3. Synthetic optimizer micro-benchmarks for algorithm overhead.
4. FPS counters only as diagnostic hints.

## Hardware-adaptive calibration protocol

The calibration script uses a dedicated calibration-only input source. A tiny Win32 helper sends F24 key pulses every 50 ms while the synthetic benchmark page is focused; the page turns each pulse into a large visible canvas transition. F24 is deliberately unused by HaxBall, and this helper is never part of normal gameplay. PresentMon defines All Input To Photon as the time from the earliest keyboard or mouse interaction contributing to a displayed frame.

The calibration script:

1. probes CPU Sets, heterogeneous-core topology, hardware GPU count, display mode and AC/DC power;
2. prunes irrelevant search dimensions;
3. measures the browser-default / default-GPU / default-CPU / normal-priority baseline first;
4. tests the remaining candidate profiles in deterministic randomized order to reduce order bias;
5. captures the baseline again at the end;
6. rejects the whole session if start/end baseline median latency or frame timing changes by more than 8%;
7. analyzes one dominant ProcessID + SwapChain stream rather than mixing CEF browser/renderer/GPU presentation streams;
8. persists the winner only after optimizer feasibility and improvement checks pass.

Use AC power for maximum-performance laptop calibration.

## Captured metrics

The pinned PresentMon 2.6.0 console capture tracks display, input and GPU data by default. The analyzer prefers input-to-photon data when enough samples exist and falls back conservatively when they do not.

Relevant fields include:

- MsAllInputToPhotonLatency
- MsClickToPhotonLatency
- DisplayLatency
- MsBetweenPresents
- MsUntilDisplayed
- ProcessID
- SwapChainAddress
- HybridPresent

Input pulses make MsAllInputToPhotonLatency deterministic enough to be the preferred calibration signal instead of routinely falling back to frame-start/display latency.

The optimizer summarizes p50/p99 latency, p99 frame time, frame MAD, present-to-display timing and sustained frame-time drift.

## Stability protection

A profile can be rejected even when its average or peak FPS is higher.

Hard guards currently include:

- minimum sample count;
- maximum dropped-frame ratio where directly available;
- thermal-headroom field for measurement sources that provide it;
- maximum positive first-vs-last frame-time drift;
- repeated-baseline calibration-session drift.

This protects against "very high FPS for a short burst, then worse latency/stutter" profiles.

## Observer effect

ETW/PresentMon capture is not free. At extremely high frame rates, measurement itself can perturb scheduling. Heavy capture is therefore an **offline calibration phase** and is not kept running during normal gameplay.

## Core micro-benchmark

hax_optimizer_bench stress-tests statistics and scoring with 4096 samples per evaluation. It measures algorithm overhead only, not HaxBall or display latency.

See VALIDATION.md for the current CI number.

## Required evidence for cross-client claims

A defensible public claim that Hax is faster than ordinary Chrome or another client needs repeatable captures across:

- multiple Intel/AMD CPU generations;
- homogeneous and hybrid CPUs;
- desktop and muxless/hybrid laptops;
- integrated and discrete GPU paths;
- multiple display refresh-rate classes;
- representative Windows/GPU-driver versions.

Until such data exists, the project makes per-machine measured selections rather than universal marketing claims.
