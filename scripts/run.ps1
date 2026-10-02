param(
  [int]$CalibrationSeconds = 10,
  [string]$PresentMon = ""
)

# Optional full latency measurement ("Recalibrate Haxball App.cmd"), then
# starts the game. Haxball App.exe never needs it: its first launch stores the
# measured default profile instantly. Runs under Windows PowerShell 5.1: keep
# every cmdlet and parameter 5.1-compatible.
$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$sourceExe = Join-Path $root "build/Release/Haxball App.exe"
$portableExe = Join-Path $root "Haxball App.exe"
$legacyPackagedExe = Join-Path $root "bin/Haxball App.exe"

if (Test-Path $sourceExe) {
  $exe = $sourceExe
}
elseif (Test-Path $portableExe) {
  $exe = $portableExe
}
elseif (Test-Path $legacyPackagedExe) {
  $exe = $legacyPackagedExe
}
else {
  throw "Haxball App.exe was not found."
}

$appData = Join-Path $env:LOCALAPPDATA "HaxballApp"
New-Item -ItemType Directory -Force -Path $appData | Out-Null

$errorPath = Join-Path $appData "last-calibration-error.txt"

# One calibration at a time.
$mutex = New-Object System.Threading.Mutex($false, "Local\HaxballAppBootstrap")
try {
  $ownsMutex = $mutex.WaitOne(0)
}
catch [System.Threading.AbandonedMutexException] {
  $ownsMutex = $true
}
if (-not $ownsMutex) {
  exit 0
}

# Calibration closes every Haxball App window, so never run it while the game
# is open; the launch below then just focuses the running game.
$gameRunning = [bool](
  Get-Process -Name "Haxball App" -ErrorAction SilentlyContinue |
    Where-Object { $_.MainWindowHandle -ne [IntPtr]::Zero })

function Show-CalibrationNotice {
  # Separate process: calibration force-closes "Haxball App" processes, and the
  # caption must differ from the benchmark window title the input helper finds.
  $text =
    "Haxball App bu bilgisayar için optimize ediliyor.`n`n" +
    "Birkaç dakika boyunca ölçüm pencereleri açılıp kapanacak. " +
    "En doğru sonuç için bu sırada bilgisayarı kullanmayın.`n`n" +
    "Bitince oyun kendiliğinden açılır."
  $noticeScript =
    "Add-Type -AssemblyName PresentationFramework; " +
    "[void][System.Windows.MessageBox]::Show('$text', " +
    "'Haxball App - Hazırlanıyor', 'OK', 'Information')"
  $encoded = [Convert]::ToBase64String(
    [Text.Encoding]::Unicode.GetBytes($noticeScript))

  try {
    return Start-Process powershell.exe -PassThru -WindowStyle Hidden `
      -ArgumentList "-NoProfile -EncodedCommand $encoded"
  }
  catch {
    return $null
  }
}

try {
  if (-not $gameRunning) {
    $calibrate = Join-Path $PSScriptRoot "calibrate.ps1"

    if (Test-Path $calibrate) {
      $notice = Show-CalibrationNotice
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
          "Time: $((Get-Date).ToUniversalTime().ToString('o'))"
          "Message: $($_.Exception.Message)"
          ""
          $_.ScriptStackTrace
        ) | Set-Content -Path $errorPath -Encoding utf8
        # The current profile.ini stays; calibrate.ps1 only replaces it on
        # success.
      }
      finally {
        if ($notice -and -not $notice.HasExited) {
          Stop-Process -Id $notice.Id -Force -ErrorAction SilentlyContinue
        }
      }
    }
  }
}
finally {
  # The game must open even if anything above failed.
  Start-Process -FilePath $exe
  $mutex.ReleaseMutex()
}
