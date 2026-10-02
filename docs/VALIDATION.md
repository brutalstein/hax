# Validation record

Date: 2026-10-02

## Portable build/test

Environment used for the initial repository validation:

- Linux container
- GCC 14.2.0
- CMake
- C++20
- HAX_BUILD_APP=OFF
- tests and optimizer benchmark enabled

Commands:

```text
cmake -S . -B build -DHAX_BUILD_APP=OFF -DHAX_BUILD_TESTS=ON -DHAX_BUILD_BENCHMARKS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/hax_optimizer_bench
```

Observed result:

```text
100% tests passed, 0 tests failed out of 1
evaluations=192 samples/eval=4096 total_ms=796.100 us/eval=4146.354 checksum=176.821
```

Interpretation:

- the portable core compiles under the repository warning profile;
- statistical/scoring invariants pass their unit tests;
- the 24-candidate worst-case search-space scoring cost is tiny compared with multi-second hardware capture windows;
- this synthetic micro-benchmark is not evidence of HaxBall input-latency improvement.

## Windows / CEF validation

The repository includes a Visual Studio 2022 x64 GitHub Actions build with the pinned CEF distribution. Its status is the compile gate for the native browser application.

## Hardware latency validation

No generic hardware-latency number is claimed here. Those results must come from PresentMon captures on specific machines using the protocol in BENCHMARKING.md.
