# Algorithm analysis

## Objective

The optimizer minimizes robust latency and instability subject to hard feasibility constraints.

For each candidate c, samples are summarized as:

- L50: median PC/input latency;
- L99: p99 PC/input latency;
- F99: p99 frame time;
- J: median absolute deviation of frame time;
- P95: p95 present-to-display time;
- CPU/GPU utilization;
- thermal headroom;
- dropped-frame ratio.

A candidate is rejected when:

```text
sample_count < N_min
dropped_ratio > D_max
thermal_headroom < T_min
timings invalid
```

Every timing metric is normalized against baseline:

```text
r(x) = x_candidate / max(abs(x_baseline), epsilon)
```

The implemented cost is:

```text
J(c) =
  0.34 r(L50)
+ 0.26 r(L99)
+ 0.16 r(F99)
+ 0.10 r(MAD)
+ 0.08 r(P95)
+ 0.03 CPU_penalty
+ 0.02 GPU_penalty
+ 0.01 thermal_penalty
+ 0.20 uncertainty
```

The uncertainty term is:

```text
1.96 * standard_error(latency) / baseline_L50
```

This prevents a noisy short run from winning because of a lucky median. The incumbent is replaced only when improvement exceeds the configured hysteresis margin.

## Robust statistics

The implementation does not assume Gaussian frame-time distributions.

- p50 represents normal experience;
- p99 protects against tail spikes;
- MAD is robust to isolated outliers;
- standard error is used only as a confidence penalty.

For m timing samples, vectors are sorted once:

```text
time  O(m log m)
space O(m)
```

Percentiles after sorting are O(1).

Calibration sample sizes are bounded, so this predictable method is preferred over approximate streaming quantiles.

## Candidate generation

The production-safe control space contains only settings actually applied by CEF/Windows:

```text
frame:    browser default | uncapped
GPU:      default | low-power | high-performance
CPU:      default | performance CPU Sets
priority: normal | above-normal | high
```

If there are F, G, C and P alternatives:

```text
candidate count = F * G * C * P
generation      = O(FGCP)
```

Current upper bound is 2 * 3 * 2 * 3 = 36 candidates.

Fixed 500/1000/2000/3000 FPS caps are not faked with JavaScript sleeping or gameplay-input modifications. Precise arbitrary capping requires a dedicated Chromium frame-pacing mechanism. Uncapped mode can still reach those frame rates when hardware allows.

## Selection

After candidates are summarized, choosing the best feasible candidate is a single scan:

```text
time  O(n)
space O(1)
```

FPS itself is a control variable, never the objective.
