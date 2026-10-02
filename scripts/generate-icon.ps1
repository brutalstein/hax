param(
  [Parameter(Mandatory=$true)]
  [string]$SourceBase64,

  [Parameter(Mandatory=$true)]
  [string]$OutputIcon
)

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing

if (-not (Test-Path $SourceBase64)) {
  throw "Base64 icon source not found: $SourceBase64"
}

function Write-ClassicIconImage {
  param(
    [Parameter(Mandatory=$true)]
    [System.Drawing.Bitmap]$Bitmap
  )

  $size = $Bitmap.Width
  $maskRowBytes = [int]([Math]::Ceiling($size / 32.0) * 4)

  $stream = New-Object System.IO.MemoryStream
  $writer = New-Object System.IO.BinaryWriter($stream)

  try {
    $writer.Write([uint32]40)
    $writer.Write([int32]$size)
    $writer.Write([int32]($size * 2))
    $writer.Write([uint16]1)
    $writer.Write([uint16]32)
    $writer.Write([uint32]0)
    $writer.Write([uint32]($size * $size * 4))
    $writer.Write([int32]0)
    $writer.Write([int32]0)
    $writer.Write([uint32]0)
    $writer.Write([uint32]0)

    for ($y = $size - 1; $y -ge 0; --$y) {
      for ($x = 0; $x -lt $size; ++$x) {
        $pixel = $Bitmap.GetPixel($x, $y)
        $writer.Write([byte]$pixel.B)
        $writer.Write([byte]$pixel.G)
        $writer.Write([byte]$pixel.R)
        $writer.Write([byte]$pixel.A)
      }
    }

    for ($y = $size - 1; $y -ge 0; --$y) {
      [byte[]]$mask = New-Object byte[] $maskRowBytes

      for ($x = 0; $x -lt $size; ++$x) {
        $pixel = $Bitmap.GetPixel($x, $y)
        if ($pixel.A -lt 128) {
          $byteIndex = [int][Math]::Floor($x / 8.0)
          $bitIndex = 7 - ($x % 8)
          $mask[$byteIndex] = [byte](
            $mask[$byteIndex] -bor (1 -shl $bitIndex)
          )
        }
      }

      $writer.Write($mask)
    }

    $writer.Flush()
    return $stream.ToArray()
  }
  finally {
    $writer.Dispose()
    $stream.Dispose()
  }
}

$base64 = (Get-Content -LiteralPath $SourceBase64 -Raw).Trim()
try {
  [byte[]]$pngBytes = [Convert]::FromBase64String($base64)
}
catch {
  throw "Icon source is not valid base64."
}

$imageStream = New-Object System.IO.MemoryStream(,$pngBytes)
$sourceImage = [System.Drawing.Image]::FromStream(
  $imageStream,
  $true,
  $true
)

$images = New-Object System.Collections.Generic.List[object]

try {
  foreach ($size in @(16, 24, 32, 48, 64, 128, 256)) {
    $bitmap = New-Object System.Drawing.Bitmap(
      $size,
      $size,
      [System.Drawing.Imaging.PixelFormat]::Format32bppArgb
    )

    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)

    try {
      $graphics.Clear([System.Drawing.Color]::Transparent)
      $graphics.CompositingMode =
        [System.Drawing.Drawing2D.CompositingMode]::SourceCopy
      $graphics.CompositingQuality =
        [System.Drawing.Drawing2D.CompositingQuality]::HighQuality
      $graphics.InterpolationMode =
        [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
      $graphics.SmoothingMode =
        [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
      $graphics.PixelOffsetMode =
        [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality

      $graphics.DrawImage(
        $sourceImage,
        0,
        0,
        $size,
        $size
      )
    }
    finally {
      $graphics.Dispose()
    }

    try {
      $bytes = Write-ClassicIconImage -Bitmap $bitmap
      $images.Add([pscustomobject]@{
        Size = $size
        Bytes = $bytes
      })
    }
    finally {
      $bitmap.Dispose()
    }
  }
}
finally {
  $sourceImage.Dispose()
  $imageStream.Dispose()
}

$outputDirectory = Split-Path -Parent $OutputIcon
if ($outputDirectory) {
  New-Item -ItemType Directory -Force -Path $outputDirectory | Out-Null
}

$fileStream = [System.IO.File]::Open(
  $OutputIcon,
  [System.IO.FileMode]::Create,
  [System.IO.FileAccess]::Write,
  [System.IO.FileShare]::None
)
$fileWriter = New-Object System.IO.BinaryWriter($fileStream)

try {
  $fileWriter.Write([uint16]0)
  $fileWriter.Write([uint16]1)
  $fileWriter.Write([uint16]$images.Count)

  $offset = 6 + (16 * $images.Count)

  foreach ($image in $images) {
    $dimension = if ($image.Size -eq 256) { 0 } else { $image.Size }

    $fileWriter.Write([byte]$dimension)
    $fileWriter.Write([byte]$dimension)
    $fileWriter.Write([byte]0)
    $fileWriter.Write([byte]0)
    $fileWriter.Write([uint16]1)
    $fileWriter.Write([uint16]32)
    $fileWriter.Write([uint32]$image.Bytes.Length)
    $fileWriter.Write([uint32]$offset)

    $offset += $image.Bytes.Length
  }

  foreach ($image in $images) {
    $fileWriter.Write([byte[]]$image.Bytes)
  }

  $fileWriter.Flush()
}
finally {
  $fileWriter.Dispose()
  $fileStream.Dispose()
}

if (-not (Test-Path $OutputIcon)) {
  throw "Failed to generate Windows icon: $OutputIcon"
}

Write-Host "Generated Haxball App icon: $OutputIcon"
