# Runs ShadowbaneFPS automation tests headlessly via UnrealEditor-Cmd.
# Usage:
#   .\scripts\RunAutomationTests.ps1
#   .\scripts\RunAutomationTests.ps1 -EngineRoot "D:\UE\UE_5.5"

param(
    [string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.5",
    [string]$Filter = "ShadowbaneFPS"
)

$ErrorActionPreference = "Stop"
$ProjectRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$UProject = Join-Path $ProjectRoot "ShadowbaneFPS.uproject"
$EditorCmd = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"

if (-not (Test-Path $UProject)) {
    throw "Project not found: $UProject"
}
if (-not (Test-Path $EditorCmd)) {
    throw "UnrealEditor-Cmd not found at $EditorCmd — pass -EngineRoot to your UE 5.5 install."
}

Write-Host "Project : $UProject"
Write-Host "Engine  : $EngineRoot"
Write-Host "Filter  : $Filter"
Write-Host ""

$Exec = "Automation RunTests $Filter; Quit"
& $EditorCmd `
    $UProject `
    -NullRHI `
    -Unattended `
    -NoSound `
    -NoSplash `
    -stdout `
    -FullStdOutLogOutput `
    -ExecCmds="$Exec"

$Code = $LASTEXITCODE
if ($Code -ne 0) {
    Write-Error "Automation finished with exit code $Code. Check Saved/Logs for details."
    exit $Code
}

Write-Host "Automation finished OK (exit $Code)."
exit 0
