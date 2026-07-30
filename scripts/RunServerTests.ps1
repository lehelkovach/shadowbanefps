# Runs ShadowbaneFPS *server* automation tests headlessly.
# Usage:
#   .\scripts\RunServerTests.ps1
#   .\scripts\RunServerTests.ps1 -EngineRoot "D:\UE\UE_5.5"

param(
    [string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.5"
)

$ErrorActionPreference = "Stop"
& (Join-Path $PSScriptRoot "RunAutomationTests.ps1") -EngineRoot $EngineRoot -Filter "ShadowbaneFPS.Server"
