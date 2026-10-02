param(
  [switch]$Recalibrate,
  [switch]$SkipCalibration,
  [int]$CalibrationSeconds = 10,
  [string]$PresentMon = ""
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$sourceExe = Join-Path $root "build/Release/Haxball App.exe"
$portableExe = Join-Path $root "Haxball App.exe"
$legacyPackagedExe = Join-Path $root "bin/Haxball App.exe"

if (Test-Path $sourceExe) {
  $exe = $sourceExe
  $binaryDir = Split-Path -Parent $sourceExe
}
elseif (Test-Path $portableExe) {
  $exe = $portableExe
  $binaryDir = $root
}
elseif (Test-Path $legacyPackagedExe) {
  $exe = $legacyPackagedExe
  $binaryDir = Split-Path -Parent $legacyPackagedExe
}
else {
  throw "Haxball App.exe was not found."
}

$appData = Join-Path $env:LOCALAPPDATA "HaxballApp"
New-Item -ItemType Directory -Force -Path $appData | Out-Null

$probe = Join-Path $binaryDir "hax_system_probe.exe"
$fingerprintScript = Join-Path $PSScriptRoot "hardware-fingerprint.ps1"
$profilePath = Join-Path $appData "profile.ini"
$errorPath = Join-Path $appData "last-calibration-error.txt"

$currentFingerprint = $null
if ((Test-Path $probe) -and (Test-Path $fingerprintScript)) {
  try {
    $currentFingerprint = & $fingerprintScript -ProbeExecutable $probe |
      Select-Object -Last 1
  }
  catch {
    $currentFingerprint = $null
  }
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

if ($needsCalibration -and -not $SkipCalibration) {
  $calibrate = Join-Path $PSScriptRoot "calibrate.ps1"

  if (Test-Path $calibrate) {
    try {
      $calibrationArgs = @{
        Seconds = $CalibrationSeconds
        Configuration = "Release"
      }

      if (-not [string]::IsNullOrWhiteSpace($PresentMon)) {
        $calibrationArgs["PresentMon"] = $PresentMon
      }

      & $calibrate @calibrationArgs

      if (Test-Path $errorPath) {
        Remove-Item $errorPath -Force -ErrorAction SilentlyContinue
      }
    }
    catch {
      @(
        "Haxball App calibration failed."
        "Time: $(Get-Date -AsUTC -Format o)"
        "Message: $($_.Exception.Message)"
        ""
        $_.ScriptStackTrace
      ) | Set-Content -Path $errorPath -Encoding utf8
    }
  }
}

Start-Process -FilePath $exe -ArgumentList "--hax-bootstrap-complete"
