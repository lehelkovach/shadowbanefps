# Local client+server debug: build both, start dedicated server, start one client.
#
# Usage:
#   .\scripts\Debug-Local.ps1
#   .\scripts\Debug-Local.ps1 -Port 7777 -NoBuild
#   .\scripts\Debug-Local.ps1 -OpenVS

param(
    [string]$EngineRoot = "",
    [int]$Port = 7777,
    [ValidateSet("Development", "DebugGame")]
    [string]$Config = "Development",
    [switch]$NoBuild,
    [switch]$OpenVS
)

$ErrorActionPreference = "Stop"

if (-not $NoBuild) {
    & (Join-Path $PSScriptRoot "Build.ps1") -Target All -Config $Config -EngineRoot $EngineRoot
}

& (Join-Path $PSScriptRoot "Run-Server.ps1") -EngineRoot $EngineRoot -Port $Port -VerboseLogs
Start-Sleep -Seconds 2
& (Join-Path $PSScriptRoot "Run-Game.ps1") -EngineRoot $EngineRoot -Server "127.0.0.1:$Port" -VerboseLogs

if ($OpenVS) {
    & (Join-Path $PSScriptRoot "Open-VS.ps1") -EngineRoot $EngineRoot
}

Write-Host ""
Write-Host "Local dedicated server + client up on 127.0.0.1:$Port"
Write-Host "Attach VS to ShadowbaneFPSServer.exe and/or the game/editor process."
Write-Host "For live OCI DEV instead: .\scripts\Connect-DevServer.ps1"
