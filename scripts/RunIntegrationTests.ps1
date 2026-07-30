# Runs ShadowbaneFPS *integration* automation tests headlessly.
# Usage:
#   .\scripts\RunIntegrationTests.ps1
#   .\scripts\RunIntegrationTests.ps1 -EngineRoot "D:\UE\UE_5.5"

param(
    [string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.5"
)

$ErrorActionPreference = "Stop"
& (Join-Path $PSScriptRoot "RunAutomationTests.ps1") -EngineRoot $EngineRoot -Filter "ShadowbaneFPS.Integration"
