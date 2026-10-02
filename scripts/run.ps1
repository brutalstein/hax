param(
  [switch]$Recalibrate,
  [switch]$SkipCalibration,
  [int]$CalibrationSeconds = 10,
  [string]$PresentMon = ""
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$sourceExe = Join-Path $root "build/Release/hax.exe"
$packagedExe = Join-Path $root "bin/hax.exe"

if (Test-Path $sourceExe) {
  $exe = $sourceExe
  $binaryDir = Split-Path -Parent $sourceExe
}
elseif (Test-Path $packagedExe) {
  $exe = $packagedExe
  $binaryDir = Split-Path -Parent $packagedExe
}
else {
  throw "hax.exe was not found. Build the app or use the packaged distribution."
}

$probe = Join-Path $binaryDir "hax_system_probe.exe"
$fingerprintScript = Join-Path $PSScriptRoot "hardware-fingerprint.ps1"
$profilePath = Join-Path $env:LOCALAPPDATA "HaxPerformanceRuntime/profile.ini"

$currentFingerprint = $null
if ((Test-Path $probe) -and (Test-Path $fingerprintScript)) {
  $currentFingerprint = & $fingerprintScript -ProbeExecutable $probe |
    Select-Object -Last 1
}

$storedFingerprint = $null
if (Test-Path $profilePath) {
  $storedFingerprint = Get-Content $profilePath |
    Where-Object { $_ -like "fingerprint=*" } |
    Select-Object -Last 1

  if ($storedFingerprint) {
    $storedFingerprint = $storedFingerprint.Substring("fingerprint=".Length)
  }
}

$profileMissing = -not (Test-Path $profilePath)
$fingerprintMissing = [string]::IsNullOrWhiteSpace($storedFingerprint)
$fingerprintChanged =
  (-not [string]::IsNullOrWhiteSpace($currentFingerprint)) -and
  ($storedFingerprint -ne $currentFingerprint)

$needsCalibration =
  $Recalibrate -or
  $profileMissing -or
  $fingerprintMissing -or
  $fingerprintChanged

if ($fingerprintChanged -and -not $Recalibrate) {
  Write-Host "Hardware/runtime fingerprint changed; recalibration is required."
}

if ($needsCalibration -and -not $SkipCalibration) {
  $calibrate = Join-Path $PSScriptRoot "calibrate.ps1"
  if (-not (Test-Path $calibrate)) {
    throw "Calibration script is missing: $calibrate"
  }

  $calibrationArgs = @{
    Seconds = $CalibrationSeconds
    Configuration = "Release"
  }

  if (-not [string]::IsNullOrWhiteSpace($PresentMon)) {
    $calibrationArgs["PresentMon"] = $PresentMon
  }

  & $calibrate @calibrationArgs
}

Start-Process -FilePath $exe
