param(
  [string]$DestinationDirectory = ""
)

$ErrorActionPreference = "Stop"

$version = "2.6.0"
$fileName = "PresentMon-$version-x64.exe"
$url = "https://github.com/GameTechDev/PresentMon/releases/download/v$version/$fileName"
$expectedSha256 = "b2a706bc6ad475749e3b7e3409263aa1e6906d45bdcf993f6dbc0f660188f1af"

if ([string]::IsNullOrWhiteSpace($DestinationDirectory)) {
  $DestinationDirectory = Join-Path $env:LOCALAPPDATA "HaxballApp/tools"
}

New-Item -ItemType Directory -Force -Path $DestinationDirectory | Out-Null
$destination = Join-Path $DestinationDirectory $fileName

function Test-PresentMonHash {
  param([string]$Path)

  if (-not (Test-Path $Path)) {
    return $false
  }

  $actual = (Get-FileHash -Algorithm SHA256 -Path $Path).Hash.ToLowerInvariant()
  return $actual -eq $expectedSha256
}

if (-not (Test-PresentMonHash -Path $destination)) {
  if (Test-Path $destination) {
    Remove-Item $destination -Force
  }

  Write-Host "Downloading official PresentMon $version..."
  Invoke-WebRequest -Uri $url -OutFile $destination -UseBasicParsing

  if (-not (Test-PresentMonHash -Path $destination)) {
    Remove-Item $destination -Force -ErrorAction SilentlyContinue
    throw "PresentMon SHA-256 verification failed."
  }
}

Write-Output $destination
