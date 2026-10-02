param(
  [switch]$NoLaunch
)

$ErrorActionPreference = "Stop"

$sourceRoot = Split-Path -Parent $PSScriptRoot
$installRoot = Join-Path $env:LOCALAPPDATA "Programs\Haxball App"

$sourceFull = [IO.Path]::GetFullPath($sourceRoot).TrimEnd("\")
$installFull = [IO.Path]::GetFullPath($installRoot).TrimEnd("\")

if ($sourceFull -ne $installFull) {
  New-Item -ItemType Directory -Force -Path $installRoot | Out-Null

  Get-ChildItem -LiteralPath $sourceRoot -Force | ForEach-Object {
    Copy-Item -LiteralPath $_.FullName -Destination $installRoot -Recurse -Force
  }
}

$exe = Join-Path $installRoot "Haxball App.exe"

if (-not (Test-Path $exe)) {
  throw "Installed Haxball App executable was not found: $exe"
}

function New-HaxballShortcut {
  param(
    [Parameter(Mandatory=$true)]
    [string]$Path
  )

  $shell = New-Object -ComObject WScript.Shell
  $shortcut = $shell.CreateShortcut($Path)
  $shortcut.TargetPath = $exe
  $shortcut.Arguments = ""
  $shortcut.WorkingDirectory = $installRoot
  $shortcut.IconLocation = "$exe,0"
  $shortcut.Description = "Launch Haxball App"
  $shortcut.Save()
}

$desktop = [Environment]::GetFolderPath("Desktop")
$startMenu = Join-Path $env:APPDATA "Microsoft\Windows\Start Menu\Programs"

New-HaxballShortcut -Path (Join-Path $desktop "Haxball App.lnk")
New-HaxballShortcut -Path (Join-Path $startMenu "Haxball App.lnk")

Write-Host "Haxball App installed to: $installRoot"
Write-Host "Desktop and Start Menu shortcuts created."

if (-not $NoLaunch) {
  Start-Process -FilePath $exe
}
