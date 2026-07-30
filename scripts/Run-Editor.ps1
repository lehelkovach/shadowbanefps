# Launch Unreal Editor for ShadowbaneFPS (PIE / content work).
#
# Usage:
#   .\scripts\Run-Editor.ps1
#   .\scripts\Run-Editor.ps1 -VerboseLogs
#   .\scripts\Run-Editor.ps1 -Build
#   .\scripts\Run-Editor.ps1 -Wait   # block until editor exits

param(
    [string]$EngineRoot = "",
    [switch]$Build,
    [switch]$VerboseLogs,
    [switch]$Wait,
    [string]$ExtraArgs = ""
)

$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "lib\UeCommon.ps1")

$ProjectRoot = Get-SBProjectRoot
$UProject = Get-SBUproject -ProjectRoot $ProjectRoot
$Engine = Resolve-SBEngineRoot -EngineRoot $EngineRoot

if ($Build) {
    & (Join-Path $PSScriptRoot "Build.ps1") -Target Editor -EngineRoot $Engine
}

$Editor = Get-SBEditorExe -EngineRoot $Engine
$LogCmds = Get-SBDefaultLogCmds -VerboseLogs:$VerboseLogs

Write-Host "Launching Editor: $UProject"
$argList = @(
    $UProject,
    "-log",
    "-LogCmds=$LogCmds"
)
if ($ExtraArgs) {
    $argList += $ExtraArgs
}

if ($Wait) {
    & $Editor @argList
    exit $LASTEXITCODE
}

Start-Process -FilePath $Editor -ArgumentList $argList -WorkingDirectory $ProjectRoot | Out-Null
Write-Host "Editor started (detached). Attach VS via .\scripts\Open-VS.ps1 then Debug → Attach to Process → UnrealEditor.exe"
