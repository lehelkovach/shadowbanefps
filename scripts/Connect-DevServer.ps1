# Launch UE client against the live DEV dedicated server.
# Usage:
#   .\scripts\Connect-DevServer.ps1
#   .\scripts\Connect-DevServer.ps1 -Server "144.24.46.16:7777"

param(
    [string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.5",
    [string]$Server = "144.24.46.16:7777"
)

$ErrorActionPreference = "Stop"
$ProjectRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$UProject = Join-Path $ProjectRoot "ShadowbaneFPS.uproject"
$Editor = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor.exe"

if (-not (Test-Path $UProject)) { throw "Missing $UProject" }
if (-not (Test-Path $Editor)) { throw "Missing editor at $Editor" }

Write-Host "Connecting to DEV server $Server ..."
& $Editor $UProject $Server -game -log
