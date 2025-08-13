param(
  [Parameter(Mandatory=$true)][string]$InTmx,
  [Parameter(Mandatory=$true)][string]$OutTmx,
  [int]$BorderTiles = 2,
  [switch]$Overwrite
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if (-not (Test-Path -LiteralPath $InTmx)) { throw "Input not found: $InTmx" }
if ((Test-Path -LiteralPath $OutTmx) -and -not $Overwrite) { throw "Out exists: $OutTmx (use -Overwrite)" }

$xml = Get-Content -LiteralPath $InTmx -Raw

# Extract dimensions
if ($xml -notmatch 'width="(\d+)"') { throw 'No width' } else { $w=[int]$Matches[1] }
if ($xml -notmatch 'height="(\d+)"') { throw 'No height' } else { $h=[int]$Matches[1] }

# Pull plat and lava CSVs
function Get-LayerCsv([string]$name){
  $pattern = [string]::Format('<layer[^>]*name=\"{0}\"[\s\S]*?<data encoding=\"csv\">([\s\S]*?)</data>', [regex]::Escape($name))
  $m=[regex]::Match($xml,$pattern,[System.Text.RegularExpressions.RegexOptions]::Singleline)
  if(-not $m.Success){ return $null }
  return $m.Groups[1].Value.Trim()
}
function Set-LayerCsv([string]$name,[string]$csv){
  $pattern = [string]::Format('(<layer[^>]*name=\"{0}\"[\s\S]*?<data encoding=\"csv\">)([\s\S]*?)(</data>)', [regex]::Escape($name))
  $xml = [regex]::Replace($script:xml,$pattern,('$1`n'+$csv+'`n$3'),[System.Text.RegularExpressions.RegexOptions]::Singleline)
  $script:xml = $xml
}

$platCsv = Get-LayerCsv 'plat'
$lavaCsv = Get-LayerCsv 'lava'
if(-not $platCsv -or -not $lavaCsv){ throw 'Missing plat or lava layer' }

function CsvToGrid([string]$csv){
  $rows = $csv -split "\r?\n" | Where-Object { $_ -ne '' }
  $grid = @()
  for($y=0;$y -lt $h;$y++){
    $vals = ($rows[$y].Trim().Trim(',')).Split(',')
    $row = New-Object 'System.Collections.Generic.List[int]'
    for($x=0;$x -lt $w;$x++){ $row.Add([int]$vals[$x]) }
    $grid += ,($row.ToArray())
  }
  return ,$grid
}
function GridToCsv($grid){
  $rows=@()
  for($y=0;$y -lt $h;$y++){
    $line=@()
    for($x=0;$x -lt $w;$x++){ $line += [string]$grid[$y][$x] }
    $rows += ($line -join ',')
  }
  return ($rows -join "`n")
}

$plat = CsvToGrid $platCsv
$lava = CsvToGrid $lavaCsv

# Carve walkable path: for any lava tile, set adjacent 1-tile ring to platform to reduce noise
for($y=0;$y -lt $h;$y++){
  for($x=0;$x -lt $w;$x++){
    if($lava[$y][$x] -ne 0){
      for($dy=-$BorderTiles;$dy -le $BorderTiles;$dy++){
        for($dx=-$BorderTiles;$dx -le $BorderTiles;$dx++){
          $nx=$x+$dx; $ny=$y+$dy
          if($nx -ge 0 -and $nx -lt $w -and $ny -ge 0 -and $ny -lt $h){
            if($lava[$ny][$nx] -eq 0){ $plat[$ny][$nx] = 208 }
          }
        }
      }
    }
  }
}

Set-LayerCsv 'plat' (GridToCsv $plat)
Set-LayerCsv 'lava' (GridToCsv $lava)

Set-Content -LiteralPath $OutTmx -Value $xml -NoNewline -Encoding UTF8
Write-Host "[tmx-post] Wrote $OutTmx"


