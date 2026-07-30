# Runs ShadowbaneFPS *client* automation tests headlessly.
# Usage:
#   .\scripts\RunClientTests.ps1
#   .\scripts\RunClientTests.ps1 -EngineRoot "D:\UE\UE_5.5"

param(
    [string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.5"
)

$ErrorActionPreference = "Stop"
& (Join-Path $PSScriptRoot "RunAutomationTests.ps1") -EngineRoot $EngineRoot -Filter "ShadowbaneFPS.Client"
