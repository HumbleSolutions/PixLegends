param(
  [Parameter(Mandatory=$true)][string]$InTmx,
  [Parameter(Mandatory=$true)][string]$OutTmx,
  [int]$ArenaRadius = 6,
  [int]$WalkwayWidth = 3,
  [switch]$Overwrite
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if (-not (Test-Path -LiteralPath $InTmx)) { throw "Input not found: $InTmx" }
if ((Test-Path -LiteralPath $OutTmx) -and -not $Overwrite) { throw "Out exists: $OutTmx (use -Overwrite)" }

$xml = Get-Content -LiteralPath $InTmx -Raw

if ($xml -notmatch 'width="(\d+)"') { throw 'No width attr' } else { $w=[int]$Matches[1] }
if ($xml -notmatch 'height="(\d+)"') { throw 'No height attr' } else { $h=[int]$Matches[1] }

function Get-LayerCsv([string]$name){
  $pattern = [string]::Format('<layer[^>]*name=\"{0}\"[\s\S]*?<data encoding=\"csv\">([\s\S]*?)</data>', [regex]::Escape($name))
  $m=[regex]::Match($xml,$pattern,[System.Text.RegularExpressions.RegexOptions]::Singleline)
  if(-not $m.Success){ return $null }
  return $m.Groups[1].Value.Trim()
}
function Set-LayerCsv([string]$name,[string]$csv){
  $pattern = [string]::Format('(<layer[^>]*name=\"{0}\"[\s\S]*?<data encoding=\"csv\">)([\s\S]*?)(</data>)', [regex]::Escape($name))
  if (-not [regex]::IsMatch($xml,$pattern,[System.Text.RegularExpressions.RegexOptions]::Singleline)) {
    # create a new empty layer before closing map
    $insert = ' <layer id="999" name="{0}" width="{1}" height="{2}">
  <data encoding="csv">
{3}
  </data>
 </layer>
'
    $insert = [string]::Format($insert, $name, $w, $h, $csv)
    $xml = ($xml -replace '</map>',$insert + '</map>')
  } else {
    $xml = [regex]::Replace($xml,$pattern,('$1`n'+$csv+'`n$3'),[System.Text.RegularExpressions.RegexOptions]::Singleline)
  }
  $script:xml = $xml
}

function CsvToGrid([string]$csv){
  $rows = $csv -split "\r?\n" | Where-Object { $_ -ne '' }
  $grid = @()
  for($y=0;$y -lt $h;$y++){
    $line = ($rows[$y]).Trim()
    $vals = $line.Trim(',').Split(',')
    $row = New-Object 'System.Collections.Generic.List[int]'
    for($x=0;$x -lt $w;$x++){
      $tok = if ($x -lt $vals.Count) { ($vals[$x]).Trim() } else { '0' }
      if ([string]::IsNullOrWhiteSpace($tok)) { $tok = '0' }
      [int]$num = [int]$tok
      $row.Add($num)
    }
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

$platCsv = Get-LayerCsv 'plat'
if(-not $platCsv){ throw 'TMX missing plat layer' }
$plat2Csv = Get-LayerCsv 'plat2'
if(-not $plat2Csv){ $plat2Csv = ('0,'*$w).Trim(',')
  for($i=1;$i -lt $h;$i++){ $plat2Csv += "`n" + ('0,'*$w).Trim(',') }
}
$lavaCsv = Get-LayerCsv 'lava'
if(-not $lavaCsv){ throw 'TMX missing lava layer' }
$stairsCsv = Get-LayerCsv 'stairs'
if(-not $stairsCsv){ $stairsCsv = ('0,'*$w).Trim(','); for($i=1;$i -lt $h;$i++){ $stairsCsv += "`n" + ('0,'*$w).Trim(',') } }

$plat = CsvToGrid $platCsv
$plat2 = CsvToGrid $plat2Csv
$lava = CsvToGrid $lavaCsv
$stairs = CsvToGrid $stairsCsv

$cx = [int]([math]::Floor($w/2))
$cy = [int]([math]::Floor($h/2))
$r = [Math]::Max(3,$ArenaRadius)
$r2 = $r*$r

# Convert circular arena to plat2 and clear lava under it
for($y=0;$y -lt $h;$y++){
  for($x=0;$x -lt $w;$x++){
    $dx=$x-$cx; $dy=$y-$cy
    if(($dx*$dx + $dy*$dy) -le $r2){
      $plat2[$y][$x] = 513   # platform2 tile
      $lava[$y][$x] = 0
      # base floor not needed directly beneath
      # keep $plat as-is
    }
  }
}

# Carve a top walkway from map top to circle
$half = [int]([math]::Floor($WalkwayWidth/2))
for($y=0;$y -lt $cy; $y++){
  for($x=$cx-$half; $x -le $cx+$half; $x++){
    if($x -ge 0 -and $x -lt $w){
      $plat[$y][$x] = 208
      $lava[$y][$x] = 0
    }
  }
}

# Place stairs where walkway meets the arena rim (one row just above the rim)
$rimY = $cy - $r
for($x=$cx-$half; $x -le $cx+$half; $x++){
  if($rimY -ge 0 -and $x -ge 0 -and $x -lt $w){ $stairs[$rimY][$x] = 609 }
}

Set-LayerCsv 'plat'   (GridToCsv $plat)
Set-LayerCsv 'plat2'  (GridToCsv $plat2)
Set-LayerCsv 'lava'   (GridToCsv $lava)
Set-LayerCsv 'stairs' (GridToCsv $stairs)

Set-Content -LiteralPath $OutTmx -Value $xml -NoNewline -Encoding UTF8
Write-Host "[tmx-arena] Wrote $OutTmx (center=$cx,$cy radius=$r)"


