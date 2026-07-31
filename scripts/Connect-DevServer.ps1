# Launch UE client against the live DEV dedicated server.
# Usage:
#   .\scripts\Connect-DevServer.ps1
#   .\scripts\Connect-DevServer.ps1 -Server "144.24.46.16:7777"
#
# Thin wrapper around Run-Game.ps1 for the OCI DEV address.

param(
    [string]$EngineRoot = "",
    [string]$Server = "144.24.46.16:7777",
    [switch]$VerboseLogs,
    [switch]$Wait,
    [string]$ExtraArgs = "-dx11"
)

$ErrorActionPreference = "Stop"
Write-Host "Connecting to DEV server $Server ..."
& (Join-Path $PSScriptRoot "Run-Game.ps1") `
    -EngineRoot $EngineRoot `
    -Server $Server `
    -VerboseLogs:$VerboseLogs `
    -Wait:$Wait `
    -ExtraArgs $ExtraArgs
