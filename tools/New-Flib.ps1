
param(
    [string]$OutputPath = "dist/flix.flib"
)

$ErrorActionPreference = "Stop"

$directory = Split-Path -Parent $OutputPath

if ($directory) {
    New-Item -ItemType Directory -Force -Path $directory | Out-Null
}

# FLIB metadata
$metadata = @{
    name        = "Flix Standard Library"
    version     = "0.1.0"
    format      = 1
    description = "Official standard library for Flix."
}

$json = $metadata | ConvertTo-Json -Compress
$metadataBytes = [System.Text.Encoding]::UTF8.GetBytes($json)

# Create binary package
$stream = [System.IO.File]::Create($OutputPath)

try {
    $writer = [System.IO.BinaryWriter]::new($stream)

    # Magic signature: FLIB
    $writer.Write([System.Text.Encoding]::ASCII.GetBytes("FLIB"))

    # Format version: uint16
    $writer.Write([uint16]1)

    # Header size: 16 bytes
    $writer.Write([uint16]16)

    # Metadata size: uint32
    $writer.Write([uint32]$metadataBytes.Length)

    # Module count: uint32
    $writer.Write([uint32]0)

    # Metadata
    $writer.Write($metadataBytes)

    $writer.Flush()
}
finally {
    if ($writer) {
        $writer.Dispose()
    }
    else {
        $stream.Dispose()
    }
}

Write-Host "Created Flix library: $OutputPath"
