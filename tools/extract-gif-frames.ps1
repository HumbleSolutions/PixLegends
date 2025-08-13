param(
    [Parameter(Mandatory=$true, Position=0)]
    [string]$InputPath,

    [Parameter(Mandatory=$false, Position=1)]
    [string]$OutputRoot = "frames",

    [switch]$Overwrite
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Write-Info($msg) { Write-Host "[gif->png] $msg" }

function Ensure-Directory($path) {
    if (-not (Test-Path -LiteralPath $path)) {
        New-Item -ItemType Directory -Path $path | Out-Null
    }
}

function Extract-GifFrames([string]$gifPath, [string]$outDir) {
    Write-Info "Extracting '$gifPath' -> '$outDir'"
    Ensure-Directory $outDir

    # Load System.Drawing types (compatible with Windows PowerShell 5.1 and PowerShell 7+)
    try { Add-Type -AssemblyName System.Drawing -ErrorAction Stop } catch {}
    try { Add-Type -AssemblyName System.Drawing.Common -ErrorAction Stop } catch {}

    try {
        $img = [System.Drawing.Image]::FromFile($gifPath)
    } catch {
        Write-Error "Failed to load GIF via System.Drawing. On PowerShell 7+, install the Windows compatibility pack: 'dotnet add package System.Drawing.Common' or run this in Windows PowerShell 5.1."
        throw
    }
    try {
        $timeDim = New-Object System.Drawing.Imaging.FrameDimension ([System.Drawing.Imaging.FrameDimension]::Time.Guid)
        $count   = $img.GetFrameCount($timeDim)
        if ($count -le 0) { Write-Info "No frames found (skipping)"; return }

        for ($i = 0; $i -lt $count; $i++) {
            $img.SelectActiveFrame($timeDim, $i) | Out-Null
            # Clone the current frame to a 32bpp ARGB bitmap to preserve alpha
            $frameBmp = New-Object System.Drawing.Bitmap($img.Width, $img.Height, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
            $gfx = [System.Drawing.Graphics]::FromImage($frameBmp)
            try {
                $gfx.DrawImage($img, 0, 0, $img.Width, $img.Height)
            } finally { $gfx.Dispose() }

            $fileName = [System.IO.Path]::GetFileNameWithoutExtension($gifPath)
            $outFile  = Join-Path $outDir ("${fileName}_frame_{0:D4}.png" -f ($i))
            if (-not $Overwrite -and (Test-Path -LiteralPath $outFile)) { continue }
            # Guard in case save fails (e.g., locked path)
            try {
                $frameBmp.Save($outFile, [System.Drawing.Imaging.ImageFormat]::Png)
            } catch {
                Write-Error ("Failed saving frame {0} to {1}: {2}" -f $i, $outFile, $_)
            }
            $frameBmp.Dispose()
        }

        Write-Info ("Saved {0} frames" -f $count)
    } finally {
        $img.Dispose()
    }
}

$resolved = Resolve-Path -LiteralPath $InputPath
$source = $resolved.Path

if (Test-Path -LiteralPath $source -PathType Container) {
    $gifs = Get-ChildItem -LiteralPath $source -Recurse -Include *.gif
    if (-not $gifs) { Write-Error "No .gif files found under '$source'" }
    foreach ($g in $gifs) {
        $subOut = Join-Path $OutputRoot ($g.BaseName)
        Ensure-Directory $subOut
        Extract-GifFrames -gifPath $g.FullName -outDir $subOut
    }
} else {
    if (-not ($source.ToLower().EndsWith('.gif'))) { Write-Error "Input must be a .gif or a directory containing .gif files" }
    $subOut = Join-Path $OutputRoot ([System.IO.Path]::GetFileNameWithoutExtension($source))
    Ensure-Directory $subOut
    Extract-GifFrames -gifPath $source -outDir $subOut
}

Write-Info "Done."


