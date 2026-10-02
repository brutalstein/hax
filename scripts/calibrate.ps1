param(
  [string]$PresentMon = "PresentMon.exe",
  [int]$Seconds = 12,
  [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$exe = Join-Path $root "build/$Configuration/hax.exe"

if (-not (Test-Path $exe)) {
  throw "hax.exe not found. Run scripts/build.ps1 -App first."
}

if (-not (Get-Command $PresentMon -ErrorAction SilentlyContinue)) {
  throw "PresentMon.exe is required. Pass -PresentMon with its path if needed."
}

$out = Join-Path $root "out/calibration"
New-Item -ItemType Directory -Force -Path $out | Out-Null

$candidates = @(
  @{ Name="browser-default"; Args=@("--hax-benchmark","--hax-fps=default","--hax-priority=normal") },
  @{ Name="uncapped-above"; Args=@("--hax-benchmark","--hax-fps=uncapped") },
  @{ Name="uncapped-highgpu"; Args=@("--hax-benchmark","--hax-fps=uncapped","--hax-gpu=high") },
  @{ Name="uncapped-lowgpu"; Args=@("--hax-benchmark","--hax-fps=uncapped","--hax-gpu=low") },
  @{ Name="uncapped-pcores"; Args=@("--hax-benchmark","--hax-fps=uncapped","--hax-cpu=performance") }
)

foreach ($candidate in $candidates) {
  $csv = Join-Path $out "$($candidate.Name).csv"
  Write-Host "Capturing $($candidate.Name)..."

  $process = Start-Process -FilePath $exe -ArgumentList $candidate.Args -PassThru
  try {
    Start-Sleep -Seconds 2
    & $PresentMon --process_id $process.Id --timed $Seconds --output_file $csv --terminate_after_timed
  }
  finally {
    if (-not $process.HasExited) {
      Stop-Process -Id $process.Id -Force
    }
  }
}

Write-Host "Calibration captures written to $out"
