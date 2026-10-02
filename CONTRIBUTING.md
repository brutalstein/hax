# Contributing

## Engineering rules

Changes must preserve the project boundary:

- do not patch, decompile, or reimplement HaxBall;
- do not inject a replacement gameplay-input path;
- do not add permanent/global Windows gaming tweaks;
- do not add a performance claim without reproducible measurements;
- prefer process-local and reversible policies;
- keep portable optimization logic independent of CEF and Windows.

## Build and test

Portable validation:

~~~powershell
cmake --preset core
cmake --build --preset core-release
ctest --preset core
~~~

Windows application:

~~~powershell
cmake --preset windows-app
cmake --build --preset windows-release
ctest --preset windows
~~~

## Performance changes

A performance-oriented change should include the hypothesis, exact hardware/software environment, baseline and candidate captures, p50/p99 latency and frame-time metrics, stability/drift data, and raw evidence sufficient to reproduce the comparison.

Average FPS alone is not evidence.

## Code style

C++ is C++20 and formatted with the repository .clang-format. Platform APIs belong behind hax_platform; statistical and selection logic belongs in hax_core.
