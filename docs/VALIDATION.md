# Validation record

Date: 2026-10-02

## CI compile and test gates

The portable core is compiled and tested on both Linux and Windows for every main-branch change.

The native Windows application is separately built on Windows Server 2022 / Visual Studio 2022 against the pinned CEF 154 distribution. The build gate verifies that the final payload contains at least:

- hax.exe
- hax_system_probe.exe
- hax_capture_analyzer.exe
- benchmark.html
- libcef.dll

The workflow also parses all PowerShell tooling before packaging the Windows artifact.

## Current optimizer micro-benchmark

A successful Ubuntu CI run after the search space was expanded to the full 36-candidate maximum reported:

~~~text
100% tests passed, 0 tests failed out of 1
evaluations=288
samples/eval=4096
total_ms=1471.885
us/eval=5110.711
checksum=264.870
~~~

Why 288 evaluations:

~~~text
36 maximum candidate profiles * 8 benchmark iterations = 288
~~~

This is a shared CI runner measurement. It proves that robust summarization/scoring overhead is small relative to multi-second real hardware captures; it is **not** a HaxBall input-latency result.

## Algorithm test coverage

The core test suite currently checks:

- percentile calculation;
- feasible baseline evaluation;
- preference for a genuinely lower-latency candidate;
- thermal-headroom rejection;
- sustained frame-time-drift rejection;
- bounded hardware-adaptive candidate generation;
- winner selection and hysteresis behavior.

## Windows / CEF issues found by CI

CI caught and the repository fixed real integration defects rather than hiding them:

1. CEF 154 no longer exposes the old CefEnableHighDPISupport helper; the app now uses current Win32 Per-Monitor V2 DPI awareness.
2. Official CEF binaries/libcef_dll_wrapper use the static MSVC runtime; all project targets now use a compatible runtime to prevent ABI/link mismatch.
3. The CEF .tar.bz2 extractor was corrected to bzip2 mode.
4. Windows CI is pinned to Visual Studio 2022, matching the supported CEF binary toolchain.

## Hardware latency validation boundary

No generic "X ms faster than Chrome" claim is made without measurements from real systems.

Per-machine claims must come from the hardware-adaptive PresentMon protocol in BENCHMARKING.md. Packaged first-run calibration stores both the selected profile and raw evidence.
