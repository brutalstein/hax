param(
  [string]$PresentMon = "",
  [int]$Seconds = 10,
  [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"

if ($Seconds -lt 5) {
  throw "Calibration captures shorter than 5 seconds are not supported."
}

$root = Split-Path -Parent $PSScriptRoot
$sourceBinaryDir = Join-Path $root "build/$Configuration"
$packagedBinaryDir = $root

if (Test-Path (Join-Path $sourceBinaryDir "Haxball App.exe")) {
  $binaryDir = $sourceBinaryDir
  $out = Join-Path $root "out/calibration"
}
elseif (Test-Path (Join-Path $packagedBinaryDir "Haxball App.exe")) {
  $binaryDir = $packagedBinaryDir
  $out = Join-Path $env:LOCALAPPDATA "HaxballApp/calibration"
}
else {
  throw "Haxball App.exe was not found. Build the app or use the packaged distribution."
}

$exe = Join-Path $binaryDir "Haxball App.exe"
$probe = Join-Path $binaryDir "hax_system_probe.exe"
$analyzer = Join-Path $binaryDir "hax_capture_analyzer.exe"
$inputPulse = Join-Path $binaryDir "hax_input_pulse.exe"

foreach ($required in @($exe, $probe, $analyzer, $inputPulse)) {
  if (-not (Test-Path $required)) {
    throw "Missing calibration artifact: $required"
  }
}

$presentMonExecutable = $null

if (-not [string]::IsNullOrWhiteSpace($PresentMon)) {
  if (Test-Path $PresentMon) {
    $presentMonExecutable = (Resolve-Path $PresentMon).Path
  }
  else {
    $presentMonCommand = Get-Command $PresentMon -ErrorAction SilentlyContinue
    if ($presentMonCommand) {
      $presentMonExecutable = $presentMonCommand.Source
    }
  }

  if (-not $presentMonExecutable) {
    throw "Requested PresentMon executable could not be resolved: $PresentMon"
  }
}
else {
  $presentMonCommand = Get-Command "PresentMon.exe" -ErrorAction SilentlyContinue
  if ($presentMonCommand) {
    $presentMonExecutable = $presentMonCommand.Source
  }
  else {
    $ensureScript = Join-Path $PSScriptRoot "ensure-presentmon.ps1"
    if (-not (Test-Path $ensureScript)) {
      throw "PresentMon is unavailable and ensure-presentmon.ps1 is missing."
    }

    $presentMonExecutable = & $ensureScript | Select-Object -Last 1
  }
}

if (-not (Test-Path $presentMonExecutable)) {
  throw "PresentMon executable is unavailable: $presentMonExecutable"
}

New-Item -ItemType Directory -Force -Path $out | Out-Null

function Stop-CalibrationProcesses {
  Get-Process -Name "Haxball App","hax_input_pulse" -ErrorAction SilentlyContinue |
    Stop-Process -Force -ErrorAction SilentlyContinue
}

function Invoke-Capture {
  param(
    [Parameter(Mandatory=$true)]
    [string]$Name,

    [Parameter(Mandatory=$true)]
    [string[]]$Arguments
  )

  $csv = Join-Path $out "$Name.csv"
  if (Test-Path $csv) {
    Remove-Item $csv -Force
  }

  Write-Host "Capturing $Name..."
  Stop-CalibrationProcesses

  $haxProcess = Start-Process -FilePath $exe -ArgumentList $Arguments -PassThru
  $pulseProcess = $null

  try {
    Start-Sleep -Milliseconds 1750

    if ($haxProcess.HasExited) {
      throw "Hax benchmark exited before capture for $Name with exit code $($haxProcess.ExitCode)."
    }

    $pulseSeconds = [Math]::Max(1, $Seconds - 1)
    $pulseArgs = @(
      "--seconds=$pulseSeconds",
      "--delay-ms=500",
      "--interval-ms=50"
    )

    $pulseProcess = Start-Process -FilePath $inputPulse -ArgumentList $pulseArgs -PassThru

    $pmArgs = @(
      "--process_name", "Haxball App.exe",
      "--timed", "$Seconds",
      "--output_file", $csv,
      "--terminate_after_timed",
      "--track_hybrid_present",
      "--no_console_stats"
    )

    & $presentMonExecutable @pmArgs

    if ($LASTEXITCODE -ne 0) {
      throw "PresentMon failed for $Name with exit code $LASTEXITCODE."
    }

    if (-not (Test-Path $csv)) {
      throw "PresentMon did not create $csv."
    }

    if ($pulseProcess -and -not $pulseProcess.HasExited) {
      [void]$pulseProcess.WaitForExit(3000)
    }

    if ($pulseProcess -and $pulseProcess.HasExited -and $pulseProcess.ExitCode -ne 0) {
      throw "Input pulse helper failed for $Name with exit code $($pulseProcess.ExitCode)."
    }

    return $csv
  }
  finally {
    if ($pulseProcess -and -not $pulseProcess.HasExited) {
      Stop-Process -Id $pulseProcess.Id -Force -ErrorAction SilentlyContinue
    }

    if ($haxProcess -and -not $haxProcess.HasExited) {
      Stop-Process -Id $haxProcess.Id -Force -ErrorAction SilentlyContinue
    }

    Stop-CalibrationProcesses
    Start-Sleep -Milliseconds 750
  }
}

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

$baselineName = "frame-default_gpu-default_cpu-default_prio-normal"
$baseline = $candidates |
  Where-Object { $_.Name -eq $baselineName } |
  Select-Object -First 1

if (-not $baseline) {
  throw "Internal error: baseline candidate was not generated."
}

$remaining = @(
  $candidates |
    Where-Object { $_.Name -ne $baselineName }
)

if ($remaining.Count -gt 1) {
  $remaining = @(
    $remaining |
      Get-Random -Count $remaining.Count -SetSeed 1548037
  )
}

$orderedCandidates = [System.Collections.Generic.List[object]]::new()
$orderedCandidates.Add($baseline)

foreach ($candidate in $remaining) {
  $orderedCandidates.Add($candidate)
}

$candidates = $orderedCandidates

Write-Host ""
Write-Host "Haxball App hardware-adaptive calibration"
Write-Host "  CPU sets: $($probeData['CPU_SET_COUNT'])"
Write-Host "  Heterogeneous CPU: $($probeData['HETEROGENEOUS_CPU'])"
Write-Host "  Hardware GPUs: $($probeData['GPU_COUNT'])"
Write-Host "  Display: $($probeData['DISPLAY_WIDTH'])x$($probeData['DISPLAY_HEIGHT']) @ $($probeData['REFRESH_HZ']) Hz"
Write-Host "  AC power: $($probeData['ON_AC'])"
Write-Host "  Candidate count: $($candidates.Count) + baseline repeat"
Write-Host "  Input pulse: F24 every 50 ms (benchmark only)"
Write-Host ""

if ($probeData["ON_AC"] -eq "0") {
  Write-Warning "Laptop is not on AC power. The resulting profile may not represent maximum-performance operation."
}

$analyzerArguments = [System.Collections.Generic.List[string]]::new()

try {
  foreach ($candidate in $candidates) {
    $csv = Invoke-Capture -Name $candidate.Name -Arguments $candidate.Args
    $analyzerArguments.Add("$($candidate.Name)=$csv")
  }

  $repeatName = "$baselineName" + "__repeat"
  $repeatCsv = Invoke-Capture -Name $repeatName -Arguments $baseline.Args
  $analyzerArguments.Add("$repeatName=$repeatCsv")
}
finally {
  Stop-CalibrationProcesses
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

$bestLine = $analysisOutput |
  Where-Object { $_ -like "BEST=*" } |
  Select-Object -Last 1

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

$fingerprintScript = Join-Path $PSScriptRoot "hardware-fingerprint.ps1"
if (-not (Test-Path $fingerprintScript)) {
  throw "Hardware fingerprint script is missing: $fingerprintScript"
}

$fingerprint = & $fingerprintScript -ProbeExecutable $probe |
  Select-Object -Last 1

if ([string]::IsNullOrWhiteSpace($fingerprint)) {
  throw "Hardware fingerprint generation failed."
}

$profileDir = Join-Path $env:LOCALAPPDATA "HaxballApp"
New-Item -ItemType Directory -Force -Path $profileDir | Out-Null
$profilePath = Join-Path $profileDir "profile.ini"

@(
  "# Generated by scripts/calibrate.ps1"
  "# $(Get-Date -AsUTC -Format o)"
  "schema=2"
  "fingerprint=$fingerprint"
  "frame=$frame"
  "gpu=$gpu"
  "cpu=$cpu"
  "priority=$priority"
) | Set-Content -Path $profilePath -Encoding ascii

Write-Host ""
Write-Host "Calibration complete."
Write-Host "Best profile: $best"
Write-Host "Persisted profile: $profilePath"
Write-Host "Raw evidence: $out"
