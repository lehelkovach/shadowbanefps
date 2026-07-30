# Open Visual Studio solution for F5 / Attach debugging.
#
# Usage:
#   .\scripts\Open-VS.ps1
#   .\scripts\Open-VS.ps1 -GenerateProjectFiles
#   .\scripts\Open-VS.ps1 -Build
#
# Debug tips:
#   1) Build Development Editor (default) — has symbols.
#   2) Set startup project to ShadowbaneFPSEditor (or launch Editor first).
#   3) Or: Debug → Attach to Process → UnrealEditor.exe / ShadowbaneFPSServer.exe
#   4) Filter Output Log: LogShadowbaneServer / Client / Net

param(
    [string]$EngineRoot = "",
    [switch]$GenerateProjectFiles,
    [switch]$Build
)

$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "lib\UeCommon.ps1")

$ProjectRoot = Get-SBProjectRoot
$UProject = Get-SBUproject -ProjectRoot $ProjectRoot
$Engine = Resolve-SBEngineRoot -EngineRoot $EngineRoot
$Sln = Join-Path $ProjectRoot "ShadowbaneFPS.sln"

if ($GenerateProjectFiles -or -not (Test-Path $Sln)) {
    Write-Host "Generating Visual Studio project files..."
    $BuildBat = Get-SBBuildBat -EngineRoot $Engine
    & $BuildBat -projectfiles "-project=$UProject" -game -engine -progress
    if ($LASTEXITCODE -ne 0) {
        throw "GenerateProjectFiles failed ($LASTEXITCODE)"
    }
}

if (-not (Test-Path $Sln)) {
    throw "Solution not found at $Sln after generate — check UE/VS toolchain."
}

if ($Build) {
    & (Join-Path $PSScriptRoot "Build.ps1") -Target Editor -EngineRoot $Engine
}

Write-Host "Opening $Sln"
Start-Process $Sln

Write-Host ""
Write-Host "Debug cheat-sheet:"
Write-Host "  • Config: Development Editor | Win64 (symbols on)"
Write-Host "  • Run Editor: .\scripts\Run-Editor.ps1"
Write-Host "  • Attach → UnrealEditor.exe  (PIE / client)"
Write-Host "  • Attach → ShadowbaneFPSServer.exe  (local dedicated)"
Write-Host "  • Or F5 with ShadowbaneFPSEditor as startup project"
Write-Host "  • Log filters: LogShadowbaneServer, LogShadowbaneClient, LogShadowbaneNet"
