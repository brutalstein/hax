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
}
elseif (Test-Path $packagedExe) {
  $exe = $packagedExe
}
else {
  throw "hax.exe was not found. Build the app or use the packaged distribution."
}

$profilePath = Join-Path $env:LOCALAPPDATA "HaxPerformanceRuntime/profile.ini"
$needsCalibration = $Recalibrate -or (-not (Test-Path $profilePath))

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
