param(
  [ValidateSet("Debug", "Release")]
  [string]$Configuration = "Release",
  [switch]$App
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$build = Join-Path $root "build"

$args = @(
  "-S", $root,
  "-B", $build,
  "-A", "x64",
  "-DHAX_BUILD_TESTS=ON",
  "-DHAX_BUILD_BENCHMARKS=ON"
)

if ($App) {
  $args += "-DHAX_BUILD_APP=ON"
}

cmake @args
