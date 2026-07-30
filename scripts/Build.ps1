# Build ShadowbaneFPS Unreal targets (Editor / Game / Server / All).
#
# Usage:
#   .\scripts\Build.ps1                          # Editor (default)
#   .\scripts\Build.ps1 -Target Game
#   .\scripts\Build.ps1 -Target Server
#   .\scripts\Build.ps1 -Target All -GenerateProjectFiles
#   .\scripts\Build.ps1 -Config DebugGame -Target Editor
#   .\scripts\Build.ps1 -EngineRoot "D:\UE\UE_5.5"
#
# Env: UE_ROOT or UE55_ROOT can replace -EngineRoot.

param(
    [ValidateSet("Editor", "Game", "Server", "All")]
    [string]$Target = "Editor",
    [ValidateSet("Development", "DebugGame", "Shipping")]
    [string]$Config = "Development",
    [string]$EngineRoot = "",
    [switch]$GenerateProjectFiles
)

$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "lib\UeCommon.ps1")

$ProjectRoot = Get-SBProjectRoot
$UProject = Get-SBUproject -ProjectRoot $ProjectRoot
$Engine = Resolve-SBEngineRoot -EngineRoot $EngineRoot

Write-Host "=== ShadowbaneFPS Build ==="
Write-Host "Project : $UProject"
Write-Host "Engine  : $Engine"
Write-Host "Target  : $Target"
Write-Host "Config  : $Config"
Write-Host ""

$gen = [bool]$GenerateProjectFiles
$map = @{
    Editor = "ShadowbaneFPSEditor"
    Game   = "ShadowbaneFPS"
    Server = "ShadowbaneFPSServer"
}

if ($Target -eq "All") {
    Invoke-SBUbtBuild -Target $map.Editor -EngineRoot $Engine -UProject $UProject -Config $Config -GenerateProjectFiles:$gen
    Invoke-SBUbtBuild -Target $map.Game   -EngineRoot $Engine -UProject $UProject -Config $Config
    Invoke-SBUbtBuild -Target $map.Server -EngineRoot $Engine -UProject $UProject -Config $Config
}
else {
    Invoke-SBUbtBuild -Target $map[$Target] -EngineRoot $Engine -UProject $UProject -Config $Config -GenerateProjectFiles:$gen
}

Write-Host ""
Write-Host "Next:"
Write-Host "  .\scripts\Run-Editor.ps1"
Write-Host "  .\scripts\Run-Game.ps1"
Write-Host "  .\scripts\Run-Server.ps1"
Write-Host "  .\scripts\Open-VS.ps1     # Visual Studio for F5 debug"
