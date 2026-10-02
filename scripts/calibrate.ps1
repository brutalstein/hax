param(
  [string]$PresentMon = "PresentMon.exe",
  [int]$Seconds = 10,
  [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"

if ($Seconds -lt 5) {
  throw "Calibration captures shorter than 5 seconds are not supported."
}

$root = Split-Path -Parent $PSScriptRoot
$exe = Join-Path $root "build/$Configuration/hax.exe"
$probe = Join-Path $root "build/$Configuration/hax_system_probe.exe"
$analyzer = Join-Path $root "build/$Configuration/hax_capture_analyzer.exe"

foreach ($required in @($exe, $probe, $analyzer)) {
  if (-not (Test-Path $required)) {
    throw "Missing build artifact: $required. Run scripts/build.ps1 -App first."
  }
}

$presentMonCommand = Get-Command $PresentMon -ErrorAction SilentlyContinue
if (-not $presentMonCommand) {
  throw "PresentMon console application is required. Pass -PresentMon with its executable path."
}

$out = Join-Path $root "out/calibration"
New-Item -ItemType Directory -Force -Path $out | Out-Null

$probeData = @{}
& $probe | ForEach-Object {
  $parts = $_ -split "=", 2
  if ($parts.Count -eq 2) {
    $probeData[$parts[0]] = $parts[1]
  }
}

$gpuPolicies = @("default")
if ([int]$probeData["GPU_COUNT"] -gt 1) {
  $gpuPolicies += @("low", "high")
}

$cpuPolicies = @("default")
if ($probeData["HETEROGENEOUS_CPU"] -eq "1") {
  $cpuPolicies += "performance"
}

$framePolicies = @("default", "uncapped")
$priorityPolicies = @("normal", "above", "high")

$candidates = [System.Collections.Generic.List[object]]::new()

foreach ($frame in $framePolicies) {
  foreach ($gpu in $gpuPolicies) {
    foreach ($cpu in $cpuPolicies) {
      foreach ($priority in $priorityPolicies) {
        $name = "frame-$frame" + "_gpu-$gpu" + "_cpu-$cpu" + "_prio-$priority"
        $args = @(
          "--hax-no-profile",
          "--hax-benchmark",
          "--hax-fps=$frame",
          "--hax-gpu=$gpu",
          "--hax-cpu=$cpu",
          "--hax-priority=$priority"
        )

        $candidates.Add([pscustomobject]@{
          Name = $name
          Args = $args
        })
      }
    }
  }
}

Write-Host ""
Write-Host "Hax hardware-adaptive calibration"
Write-Host "  CPU sets: $($probeData['CPU_SET_COUNT'])"
Write-Host "  Heterogeneous CPU: $($probeData['HETEROGENEOUS_CPU'])"
Write-Host "  Hardware GPUs: $($probeData['GPU_COUNT'])"
Write-Host "  Display: $($probeData['DISPLAY_WIDTH'])x$($probeData['DISPLAY_HEIGHT']) @ $($probeData['REFRESH_HZ']) Hz"
Write-Host "  AC power: $($probeData['ON_AC'])"
Write-Host "  Candidate count: $($candidates.Count)"
Write-Host ""

if ($probeData["ON_AC"] -eq "0") {
  Write-Warning "Laptop is not on AC power. The resulting profile may not represent maximum-performance operation."
}

Get-Process -Name "hax" -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue

$analyzerArguments = [System.Collections.Generic.List[string]]::new()

try {
  foreach ($candidate in $candidates) {
    $csv = Join-Path $out "$($candidate.Name).csv"
    if (Test-Path $csv) {
      Remove-Item $csv -Force
    }

    Write-Host "Capturing $($candidate.Name)..."

    $process = Start-Process -FilePath $exe -ArgumentList $candidate.Args -PassThru

    try {
      Start-Sleep -Seconds 2

      $pmArgs = @(
        "--process_name", "hax.exe",
        "--timed", "$Seconds",
        "--output_file", $csv,
        "--terminate_after_timed",
        "--track_hybrid_present",
        "--no_console_stats"
      )
      & $PresentMon @pmArgs

      if ($LASTEXITCODE -ne 0) {
        throw "PresentMon failed for $($candidate.Name) with exit code $LASTEXITCODE."
      }

      if (-not (Test-Path $csv)) {
        throw "PresentMon did not create $csv."
      }

      $analyzerArguments.Add("$($candidate.Name)=$csv")
    }
    finally {
      Get-Process -Name "hax" -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
      Start-Sleep -Milliseconds 750
    }
  }
}
finally {
  Get-Process -Name "hax" -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
}

Write-Host ""
Write-Host "Analyzing captures..."

$analysisOutput = & $analyzer @analyzerArguments
if ($LASTEXITCODE -ne 0) {
  $analysisOutput | ForEach-Object { Write-Host $_ }
  throw "Capture analysis failed with exit code $LASTEXITCODE."
}

$analysisOutput | ForEach-Object { Write-Host $_ }

$resultPath = Join-Path $out "result.txt"
$analysisOutput | Set-Content -Path $resultPath -Encoding utf8

$bestLine = $analysisOutput | Where-Object { $_ -like "BEST=*" } | Select-Object -Last 1

if (-not $bestLine) {
  throw "Analyzer did not report a winning candidate."
}

$best = $bestLine.Substring(5)
$parts = $best -split "_"
if ($parts.Count -ne 4) {
  throw "Unexpected winning profile name: $best"
}

$frame = $parts[0].Substring("frame-".Length)
$gpu = $parts[1].Substring("gpu-".Length)
$cpu = $parts[2].Substring("cpu-".Length)
$priority = $parts[3].Substring("prio-".Length)

$profileDir = Join-Path $env:LOCALAPPDATA "HaxPerformanceRuntime"
New-Item -ItemType Directory -Force -Path $profileDir | Out-Null
$profilePath = Join-Path $profileDir "profile.ini"

@(
  "# Generated by scripts/calibrate.ps1"
  "# $(Get-Date -AsUTC -Format o)"
  "frame=$frame"
  "gpu=$gpu"
  "cpu=$cpu"
  "priority=$priority"
) | Set-Content -Path $profilePath -Encoding utf8

Write-Host ""
Write-Host "Calibration complete."
Write-Host "Best profile: $best"
Write-Host "Persisted profile: $profilePath"
Write-Host "Raw evidence: $out"
