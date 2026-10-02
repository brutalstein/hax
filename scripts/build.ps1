param(
  [ValidateSet("Debug", "Release")]
  [string]$Configuration = "Release",
  [switch]$App
)

$ErrorActionPreference = "Stop"

& "$PSScriptRoot/configure.ps1" -Configuration $Configuration -App:$App

$root = Split-Path -Parent $PSScriptRoot
$build = Join-Path $root "build"

cmake --build $build --config $Configuration --parallel
ctest --test-dir $build -C $Configuration --output-on-failure

$bench = Join-Path $build "$Configuration/hax_optimizer_bench.exe"
if (Test-Path $bench) {
  & $bench
}
