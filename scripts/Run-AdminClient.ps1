# Launch admin spectator client with optional bot populate.
#
# Usage:
#   .\scripts\Run-AdminClient.ps1
#   .\scripts\Run-AdminClient.ps1 -Bots 8
#   .\scripts\Run-AdminClient.ps1 -Bots 10 -Build
#
# Joins as free-cam admin spectator and asks the GameMode to spawn bots
# (?Bots=N?AdminSpectate=1). Watch the siege play; pull combat_*.csv after.

param(
    [string]$EngineRoot = "",
    [int]$Bots = 8,
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

$LogCmds = Get-SBDefaultLogCmds -VerboseLogs:$VerboseLogs
$Editor = Get-SBEditorExe -EngineRoot $Engine

$UrlOpts = "?Bots=$Bots?AdminSpectate=1"
$argList = @(
    $UProject,
    $UrlOpts,
    "-game",
    "-log",
    "-LogCmds=$LogCmds"
)
if ($ExtraArgs) { $argList += $ExtraArgs }

Write-Host "Admin spectator + bots=$Bots"
Write-Host "Args: $($argList -join ' ')"
Write-Host "In-game console: AddBots N   |   AdminSpectate"
Write-Host "Telemetry: Saved\Telemetry\combat_*.csv"

if ($Wait) {
    & $Editor @argList
    exit $LASTEXITCODE
}

Start-Process -FilePath $Editor -ArgumentList $argList -WorkingDirectory $ProjectRoot | Out-Null
Write-Host "Admin client started (detached)."
