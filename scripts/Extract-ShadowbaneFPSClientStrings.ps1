#Requires -Version 5.1
<#
.SYNOPSIS
  Parse shadowbanefps client ENGLISH string dumps into Config/Shadowbane/*.json.

.DESCRIPTION
  Reads UTF-16LE DataStringENGLISH.txt / EffectsENGLISH.txt and ASCII
  ItemENGLISH.txt from Config/Shadowbane/shadowbanefps/ (or -SourceDir).

  Outputs (under Config/Shadowbane/ by default):
    powers.json, skills.json, items-catalog.json, effect-affixes.json,
    race-class-disc-labels.json

.EXAMPLE
  .\scripts\Extract-ShadowbaneFPSClientStrings.ps1
#>
[CmdletBinding()]
param(
	[string]$SourceDir = "",
	[string]$OutDir = "",
	[string]$ProjectRoot = ""
)

$ErrorActionPreference = "Stop"

if (-not $ProjectRoot) {
	$ProjectRoot = Split-Path -Parent $PSScriptRoot
}
if (-not $SourceDir) {
	$SourceDir = Join-Path $ProjectRoot "Config\Shadowbane\shadowbanefps"
}
if (-not $OutDir) {
	$OutDir = Join-Path $ProjectRoot "Config\Shadowbane"
}

function Read-ShadowbaneFPSText([string]$Path) {
	if (-not (Test-Path -LiteralPath $Path)) {
		throw "Missing source file: $Path"
	}
	$bytes = [System.IO.File]::ReadAllBytes($Path)
	# UTF-16 LE BOM
	if ($bytes.Length -ge 2 -and $bytes[0] -eq 0xFF -and $bytes[1] -eq 0xFE) {
		return [System.Text.Encoding]::Unicode.GetString($bytes, 2, $bytes.Length - 2)
	}
	# UTF-8 BOM
	if ($bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF) {
		return [System.Text.Encoding]::UTF8.GetString($bytes, 3, $bytes.Length - 3)
	}
	return [System.Text.Encoding]::ASCII.GetString($bytes)
}

function Escape-JsonString([string]$s) {
	if ($null -eq $s) { return "" }
	# shadowbanefps dumps often store the two-char sequence \n instead of a real newline.
	$s = $s -replace '\\r\\n', "`n"
	$s = $s -replace '\\n', "`n"
	$s = $s -replace '\\t', "`t"
	$s = $s -replace '\\', '\\\\'
	$s = $s -replace '"', '\"'
	$s = $s -replace "`r`n", '\n'
	$s = $s -replace "`n", '\n'
	$s = $s -replace "`r", '\n'
	$s = $s -replace "`t", '\t'
	return $s
}

function Write-JsonArrayFile([string]$Path, [System.Collections.IEnumerable]$Objects, [string[]]$Fields) {
	$dir = Split-Path -Parent $Path
	if (-not (Test-Path -LiteralPath $dir)) {
		New-Item -ItemType Directory -Force -Path $dir | Out-Null
	}
	$sw = New-Object System.IO.StreamWriter($Path, $false, [System.Text.UTF8Encoding]::new($false))
	try {
		$sw.WriteLine("[")
		$first = $true
		foreach ($obj in $Objects) {
			if (-not $first) { $sw.WriteLine(",") } else { $first = $false }
			$parts = New-Object System.Collections.Generic.List[string]
			foreach ($f in $Fields) {
				$val = $obj.$f
				if ($null -eq $val) { $val = "" }
				if ($val -is [bool] -or $val -is [int] -or $val -is [long] -or $val -is [double]) {
					$parts.Add(('"{0}":{1}' -f $f, ($val.ToString().ToLowerInvariant())))
				}
				else {
					$parts.Add(('"{0}":"{1}"' -f $f, (Escape-JsonString ([string]$val))))
				}
			}
			$sw.Write(("  {{{0}}}" -f ($parts -join ",")))
		}
		$sw.WriteLine("")
		$sw.WriteLine("]")
	}
	finally {
		$sw.Close()
	}
}

function Get-QuotedPairs([string]$Text, [string]$Prefix) {
	# "Prefix:Key" "Value"  (value may span until next closing quote; no escapes in shadowbanefps dumps)
	$pattern = '"({0}:([^"]+))"\s+"([^"]*)"' -f [regex]::Escape($Prefix)
	return [regex]::Matches($Text, $pattern)
}

Write-Host "Source: $SourceDir"
Write-Host "Output: $OutDir"

$dataPath = Join-Path $SourceDir "DataStringENGLISH.txt"
$itemPath = Join-Path $SourceDir "ItemENGLISH.txt"
$effPath = Join-Path $SourceDir "EffectsENGLISH.txt"

$data = Read-ShadowbaneFPSText $dataPath
$itemsRaw = Read-ShadowbaneFPSText $itemPath
$effects = Read-ShadowbaneFPSText $effPath

# --- Powers + PowerDescriptions ---
$descByKey = @{}
foreach ($m in (Get-QuotedPairs $data "PowerDescription")) {
	$key = $m.Groups[2].Value
	$val = $m.Groups[3].Value
	$descByKey[$key] = $val
}

$powers = New-Object System.Collections.Generic.List[object]
foreach ($m in (Get-QuotedPairs $data "Power")) {
	$id = $m.Groups[2].Value
	$name = $m.Groups[3].Value
	# Skip template / UI message strings (contain % tokens or are not ability-like)
	if ($id -match '%' -or $name -match '%') { continue }
	$desc = ""
	if ($descByKey.ContainsKey($id)) { $desc = $descByKey[$id] }
	elseif ($descByKey.ContainsKey($name)) { $desc = $descByKey[$name] }
	$powers.Add([pscustomobject]@{ id = $id; name = $name; description = $desc })
}

# --- Skills + SkillDesc ---
$skillDesc = @{}
foreach ($m in (Get-QuotedPairs $data "SkillDesc")) {
	$skillDesc[$m.Groups[2].Value] = $m.Groups[3].Value
}
$skills = New-Object System.Collections.Generic.List[object]
foreach ($m in (Get-QuotedPairs $data "Skill")) {
	$id = $m.Groups[2].Value
	$name = $m.Groups[3].Value
	$d = ""
	if ($skillDesc.ContainsKey($id)) { $d = $skillDesc[$id] }
	elseif ($skillDesc.ContainsKey($name)) { $d = $skillDesc[$name] }
	$skills.Add([pscustomobject]@{ id = $id; name = $name; description = $d })
}

# --- Race / class / disc labels ---
$labels = New-Object System.Collections.Generic.List[object]
foreach ($m in (Get-QuotedPairs $data "RaceClassDiscTalent")) {
	$id = $m.Groups[2].Value
	$name = $m.Groups[3].Value
	if ($id -eq "English Name") { continue }
	$labels.Add([pscustomobject]@{ id = $id; name = $name })
}

# --- Items (ASCII lines: <id> "Name" <Gender> "order") ---
$itemMatches = [regex]::Matches($itemsRaw, '(?m)^(\d+)\s+"([^"]+)"\s+([MFN])\s+"([^"]*)"')
$items = New-Object System.Collections.Generic.List[object]
foreach ($m in $itemMatches) {
	$idNum = [long]$m.Groups[1].Value
	$items.Add([pscustomobject]@{
			id         = $idNum
			name       = $m.Groups[2].Value
			gender     = $m.Groups[3].Value
			nameFormat = $m.Groups[4].Value
		})
}

# --- Effect affixes ---
$affixes = New-Object System.Collections.Generic.List[object]
foreach ($kind in @("EffectPrefix", "EffectSuffix")) {
	$kindShort = if ($kind -eq "EffectPrefix") { "prefix" } else { "suffix" }
	foreach ($m in (Get-QuotedPairs $effects $kind)) {
		$id = $m.Groups[2].Value
		# EffectsENGLISH often repeats the display string 3x; first quoted value is Groups[3]
		$name = $m.Groups[3].Value
		$affixes.Add([pscustomobject]@{ id = $id; kind = $kindShort; name = $name })
	}
}

$powersPath = Join-Path $OutDir "powers.json"
$skillsPath = Join-Path $OutDir "skills.json"
$itemsPath = Join-Path $OutDir "items-catalog.json"
$affixPath = Join-Path $OutDir "effect-affixes.json"
$labelsPath = Join-Path $OutDir "race-class-disc-labels.json"

Write-JsonArrayFile $powersPath $powers @("id", "name", "description")
Write-JsonArrayFile $skillsPath $skills @("id", "name", "description")
Write-JsonArrayFile $itemsPath $items @("id", "name", "gender", "nameFormat")
Write-JsonArrayFile $affixPath $affixes @("id", "kind", "name")
Write-JsonArrayFile $labelsPath $labels @("id", "name")

$withDesc = ($powers | Where-Object { $_.description }).Count
Write-Host ("powers.json: {0} ({1} with PowerDescription link)" -f $powers.Count, $withDesc)
Write-Host ("skills.json: {0}" -f $skills.Count)
Write-Host ("items-catalog.json: {0}" -f $items.Count)
Write-Host ("effect-affixes.json: {0}" -f $affixes.Count)
Write-Host ("race-class-disc-labels.json: {0}" -f $labels.Count)
Write-Host "Done."
