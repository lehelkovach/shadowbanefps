# Launch ShadowbaneFPS as a standalone game client (-game).
#
# Usage:
#   .\scripts\Run-Game.ps1                         # local standalone / offline
#   .\scripts\Run-Game.ps1 -Server 127.0.0.1:7777  # connect to local dedicated
#   .\scripts\Run-Game.ps1 -Server 144.24.46.16:7777
#   .\scripts\Run-Game.ps1 -Mode FFA -Bots 3        # offline/listen FFA smoke
#   .\scripts\Run-Game.ps1 -Build -VerboseLogs
#
# Prefers Binaries\Win64\ShadowbaneFPS.exe when present; otherwise Editor -game.

param(
    [string]$EngineRoot = "",
    [string]$Server = "",
    [int]$Bots = 0,
    [ValidateSet("", "FFA", "Deathmatch", "DM", "Siege")]
    [string]$Mode = "",
    [ValidateSet("Development", "DebugGame", "Shipping")]
    [string]$Config = "Development",
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
    & (Join-Path $PSScriptRoot "Build.ps1") -Target Game -Config $Config -EngineRoot $Engine
}

$LogCmds = Get-SBDefaultLogCmds -VerboseLogs:$VerboseLogs
$GameExe = Join-Path $ProjectRoot "Binaries\Win64\ShadowbaneFPS.exe"

# Map URL options (?Mode=FFA?Bots=N) land in GameMode::InitGame Options.
# Only used for offline / Editor -game (dedicated already has its own URL).
$mapOpts = @()
if ($Mode -and $Mode -ne "Siege") { $mapOpts += "Mode=$Mode" }
if ($Bots -gt 0) { $mapOpts += "Bots=$Bots" }
$mapUrl = "/Engine/Maps/Entry"
if ($mapOpts.Count -gt 0) {
    $mapUrl = "/Engine/Maps/Entry?" + ($mapOpts -join "?")
}

$argList = @()
$exe = $null

if (Test-Path $GameExe) {
    $exe = $GameExe
    Write-Host "Using cooked/built game binary: $GameExe"
    if ($Server) {
        $argList += $Server
    }
    elseif ($mapOpts.Count -gt 0) {
        $argList += $mapUrl
    }
    $argList += @("-log", "-LogCmds=$LogCmds")
    $argList += Get-SBDebugArgs -VerboseLogs:$VerboseLogs
}
else {
    $exe = Get-SBEditorExe -EngineRoot $Engine
    Write-Host "No Binaries\Win64\ShadowbaneFPS.exe - launching Editor -game"
    $argList += @($UProject, $mapUrl)
    if ($Server) { $argList += $Server }
    $argList += @("-game", "-log", "-LogCmds=$LogCmds")
    $argList += Get-SBDebugArgs -VerboseLogs:$VerboseLogs
}

if ($ExtraArgs) { $argList += $ExtraArgs }

Write-Host "Client args: $($argList -join ' ')"

if ($Wait) {
    & $exe @argList
    exit $LASTEXITCODE
}

Start-Process -FilePath $exe -ArgumentList $argList -WorkingDirectory $ProjectRoot | Out-Null
Write-Host "Client started (detached)."
