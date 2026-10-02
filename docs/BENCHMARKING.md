# Benchmarking and evidence

## Evidence hierarchy

1. On-device presentation/input measurements with PresentMon or equivalent ETW capture.
2. Repeatable A/B captures with the same scene, resolution, power state, thermal state and driver.
3. Synthetic optimizer micro-benchmarks for algorithm overhead.
4. FPS counters only as diagnostic hints.

## Calibration protocol

For every candidate:

1. Use AC power on laptops.
2. Close unrelated foreground workloads.
3. Warm the workload before capture.
4. Capture at least 10 seconds; longer runs are preferred for tails.
5. Alternate/randomize candidate order when doing scientific comparison.
6. Record driver, CEF, Windows build, display refresh, and GPU path.
7. Reject runs with thermal collapse or significant dropped frames.
8. Compare p50 and p99 latency/frame-time metrics, not only averages.

## PresentMon metrics

Useful columns include:

- MsPCLatency
- MsUntilDisplayed
- MsBetweenDisplayChange
- presented/displayed frame timing
- CPU/GPU utilization where available

The calibration script writes one CSV per policy.

## Observer effect

ETW capture is not free. At very high FPS, measurement can perturb scheduling. Heavy capture is therefore an offline calibration phase, not a permanent gameplay service.

## Core micro-benchmark

hax_optimizer_bench stress-tests statistics and scoring with thousands of samples per evaluation. It measures optimizer overhead only, not GPU or HaxBall performance.

## Required proof before generic performance claims

A defensible public latency claim needs multiple CPU families, desktop and hybrid-laptop systems, integrated/discrete/muxless GPU paths, several refresh-rate classes, and repeatable raw capture artifacts.
