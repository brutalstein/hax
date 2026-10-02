param(
  [Parameter(Mandatory=$true)]
  [string]$SourceIcon,

  [Parameter(Mandatory=$true)]
  [string]$OutputIcon
)

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing

if (-not (Test-Path $SourceIcon)) {
  throw "Source icon not found: $SourceIcon"
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
    # BITMAPINFOHEADER. ICO DIB height includes XOR + AND masks.
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

    # XOR bitmap: BGRA, bottom-up.
    for ($y = $size - 1; $y -ge 0; --$y) {
      for ($x = 0; $x -lt $size; ++$x) {
        $pixel = $Bitmap.GetPixel($x, $y)
        $writer.Write([byte]$pixel.B)
        $writer.Write([byte]$pixel.G)
        $writer.Write([byte]$pixel.R)
        $writer.Write([byte]$pixel.A)
      }
    }

    # AND mask: transparent pixels are 1. Rows are DWORD-aligned.
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

$iconBytes = [System.IO.File]::ReadAllBytes($SourceIcon)

if ($iconBytes.Length -lt 22) {
  throw "Source icon is too small to contain a valid ICO directory."
}

$count = [System.BitConverter]::ToUInt16($iconBytes, 4)
if ($count -lt 1) {
  throw "Source icon contains no image entries."
}

$bestOffset = 0
$bestLength = 0
$bestArea = -1

for ($index = 0; $index -lt $count; ++$index) {
  $entry = 6 + (16 * $index)
  if (($entry + 16) -gt $iconBytes.Length) {
    throw "Source icon directory is truncated."
  }

  $width = if ($iconBytes[$entry] -eq 0) {
    256
  } else {
    [int]$iconBytes[$entry]
  }

  $height = if ($iconBytes[$entry + 1] -eq 0) {
    256
  } else {
    [int]$iconBytes[$entry + 1]
  }

  $length = [System.BitConverter]::ToUInt32($iconBytes, $entry + 8)
  $offset = [System.BitConverter]::ToUInt32($iconBytes, $entry + 12)
  $area = $width * $height

  if ($area -gt $bestArea) {
    $bestArea = $area
    $bestOffset = [int]$offset
    $bestLength = [int]$length
  }
}

if (
  $bestLength -lt 8 -or
  $bestOffset -lt 0 -or
  ($bestOffset + $bestLength) -gt $iconBytes.Length
) {
  throw "Largest source icon entry has invalid bounds."
}

[byte[]]$embeddedImage = New-Object byte[] $bestLength
[System.Array]::Copy(
  $iconBytes,
  $bestOffset,
  $embeddedImage,
  0,
  $bestLength
)

$pngSignature = @(137, 80, 78, 71, 13, 10, 26, 10)
for ($i = 0; $i -lt $pngSignature.Count; ++$i) {
  if ($embeddedImage[$i] -ne $pngSignature[$i]) {
    throw "Largest source icon entry is not PNG encoded."
  }
}

$imageStream = New-Object System.IO.MemoryStream(,$embeddedImage)
$sourceImage = [System.Drawing.Image]::FromStream(
  $imageStream,
  $true,
  $true
)
$images = New-Object System.Collections.Generic.List[object]

try {
  foreach ($size in @(16, 32, 48, 256)) {
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
  if ($sourceImage) {
    $sourceImage.Dispose()
  }
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

Write-Host "Generated RC-compatible Haxball App icon: $OutputIcon"
