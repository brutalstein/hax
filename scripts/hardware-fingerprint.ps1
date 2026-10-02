param(
  [Parameter(Mandatory=$true)]
  [string]$ProbeExecutable
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path $ProbeExecutable)) {
  throw "Hardware probe executable not found: $ProbeExecutable"
}

$material = [System.Collections.Generic.List[string]]::new()

$material.Add("PROFILE_SCHEMA=3")
$material.Add("CEF=154.0.28+g564dd6c+chromium-154.0.8037.58")
$material.Add("OS=$([Environment]::OSVersion.VersionString)")

& $ProbeExecutable |
  Where-Object { $_ -notlike "ON_AC=*" } |
  Sort-Object |
  ForEach-Object { $material.Add("PROBE=$_") }

try {
  Get-CimInstance Win32_VideoController -ErrorAction Stop |
    Sort-Object PNPDeviceID |
    ForEach-Object {
      $material.Add(
        "GPU_DRIVER=$($_.PNPDeviceID)|$($_.DriverVersion)"
      )
    }
}
catch {
  $material.Add("GPU_DRIVER=unavailable")
}

try {
  Get-CimInstance -Namespace root\wmi -ClassName WmiMonitorID -ErrorAction Stop |
    Sort-Object InstanceName |
    ForEach-Object {
      $product = ($_.ProductCodeID | ForEach-Object { [char]$_ }) -join ""
      $serial = ($_.SerialNumberID | ForEach-Object { [char]$_ }) -join ""
      $material.Add(
        "MONITOR=$($_.InstanceName)|$product|$serial"
      )
    }
}
catch {
  $material.Add("MONITOR=unavailable")
}

$bytes = [Text.Encoding]::UTF8.GetBytes(
  ($material -join [Environment]::NewLine)
)

$sha256 = [Security.Cryptography.SHA256]::Create()
try {
  $hash = $sha256.ComputeHash($bytes)
}
finally {
  $sha256.Dispose()
}

($hash | ForEach-Object { $_.ToString("x2") }) -join ""
