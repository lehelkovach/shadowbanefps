# Analyze combat_*.csv for balance (build/class/power queries).
#
# Usage:
#   .\scripts\Analyze-CombatBalance.ps1
#   .\scripts\Analyze-CombatBalance.ps1 -Command class-vs-class
#   .\scripts\Analyze-CombatBalance.ps1 -Command summary -CompareLateSB
#   .\scripts\Analyze-CombatBalance.ps1 -Path "Saved\Telemetry\combat_....csv"

param(
    [ValidateSet(
        "summary",
        "build-vs-build",
        "class-vs-class",
        "power-vs-victim-arch",
        "power-vs-victim-class",
        "power-vs-armor",
        "heals",
        "query"
    )]
    [string]$Command = "summary",
    [string]$Path = "",
    [switch]$CompareLateSB,
    [string]$AtkClass = "",
    [string]$VicClass = "",
    [string]$AtkArch = "",
    [string]$VicArch = "",
    [string]$Power = "",
    [string]$PowerContains = "",
    [ValidateSet("", "Damage", "Heal", "Kill")]
    [string]$Kind = ""
)

$ErrorActionPreference = "Stop"
$ProjectRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$Py = Join-Path $ProjectRoot "tools\balance\analyze_combat.py"

if (-not $Path) {
    $Path = Join-Path $ProjectRoot "Saved\Telemetry"
}

$pyArgs = @($Py, $Command, $Path)
if ($CompareLateSB) { $pyArgs += "--compare-late-sb" }
if ($AtkClass) { $pyArgs += @("--atk-class", $AtkClass) }
if ($VicClass) { $pyArgs += @("--vic-class", $VicClass) }
if ($AtkArch) { $pyArgs += @("--atk-arch", $AtkArch) }
if ($VicArch) { $pyArgs += @("--vic-arch", $VicArch) }
if ($Power) { $pyArgs += @("--power", $Power) }
if ($PowerContains) { $pyArgs += @("--power-contains", $PowerContains) }
if ($Kind) { $pyArgs += @("--kind", $Kind) }

$python = Get-Command python -ErrorAction SilentlyContinue
if (-not $python) { $python = Get-Command python3 -ErrorAction SilentlyContinue }
if (-not $python) { throw "Python not found. Install Python 3 and retry." }

& $python.Source @pyArgs
exit $LASTEXITCODE
