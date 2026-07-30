# Local debug loop: build Editor (DebugGame optional) + launch with verbose siege logs.
#
# Usage:
#   .\scripts\Debug-Editor.ps1
#   .\scripts\Debug-Editor.ps1 -Config DebugGame
#   .\scripts\Debug-Editor.ps1 -NoBuild
#
# For breakpoints: run this, then .\scripts\Open-VS.ps1 and Attach to UnrealEditor.exe
# (or set ShadowbaneFPSEditor as VS startup and F5 instead of this script).

param(
    [string]$EngineRoot = "",
    [ValidateSet("Development", "DebugGame")]
    [string]$Config = "Development",
    [switch]$NoBuild,
    [switch]$OpenVS
)

$ErrorActionPreference = "Stop"

if (-not $NoBuild) {
    & (Join-Path $PSScriptRoot "Build.ps1") -Target Editor -Config $Config -EngineRoot $EngineRoot -GenerateProjectFiles
}

& (Join-Path $PSScriptRoot "Run-Editor.ps1") -EngineRoot $EngineRoot -VerboseLogs

if ($OpenVS) {
    & (Join-Path $PSScriptRoot "Open-VS.ps1") -EngineRoot $EngineRoot
}

Write-Host ""
Write-Host "Editor launched with verbose LogShadowbane* categories."
Write-Host "Attach debugger: .\scripts\Open-VS.ps1  →  Attach to UnrealEditor.exe"
