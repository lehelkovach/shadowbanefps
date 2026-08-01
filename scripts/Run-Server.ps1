# Launch a local Win64 dedicated server for LAN / solo testing.
#
# Usage:
#   .\scripts\Run-Server.ps1
#   .\scripts\Run-Server.ps1 -Port 7777 -Build
#   .\scripts\Run-Server.ps1 -Mode FFA
#   .\scripts\Run-Server.ps1 -Mode FFA -Bots 4 -VerboseLogs
#
# Prefers Binaries\Win64\ShadowbaneFPSServer.exe; falls back to Editor -server.
# Then connect with:  .\scripts\Run-Game.ps1 -Server 127.0.0.1:7777

param(
    [string]$EngineRoot = "",
    [int]$Port = 7777,
    [ValidateSet("Development", "DebugGame", "Shipping")]
    [string]$Config = "Development",
    [ValidateSet("", "FFA", "Deathmatch", "DM", "Siege")]
    [string]$Mode = "",
    [int]$Bots = 0,
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
    & (Join-Path $PSScriptRoot "Build.ps1") -Target Server -Config $Config -EngineRoot $Engine
}

$LogCmds = Get-SBDefaultLogCmds -VerboseLogs:$VerboseLogs
$ServerExe = Join-Path $ProjectRoot "Binaries\Win64\ShadowbaneFPSServer.exe"

# Map URL options land in GameMode::InitGame (ParseLaunchOptions).
$mapOpts = @()
if ($Mode -and $Mode -ne "Siege") { $mapOpts += "Mode=$Mode" }
if ($Bots -gt 0) { $mapOpts += "Bots=$Bots" }
$mapUrl = "/Engine/Maps/Entry"
if ($mapOpts.Count -gt 0) {
    $mapUrl = "/Engine/Maps/Entry?" + ($mapOpts -join "?")
}

$argList = @()
$exe = $null

if (Test-Path $ServerExe) {
    $exe = $ServerExe
    Write-Host "Using dedicated server binary: $ServerExe"
    $argList += @(
        $mapUrl,
        "-log",
        "-PORT=$Port",
        "-LogCmds=$LogCmds"
    )
}
else {
    $exe = Get-SBEditorExe -EngineRoot $Engine
    Write-Host "No Binaries\Win64\ShadowbaneFPSServer.exe - launching Editor -server"
    $argList += @(
        $UProject,
        $mapUrl,
        "-server",
        "-log",
        "-PORT=$Port",
        "-LogCmds=$LogCmds"
    )
}

if ($ExtraArgs) { $argList += $ExtraArgs }

Write-Host "Server listening on UDP $Port  map=$mapUrl"
if ($Mode -eq "FFA" -or $Mode -eq "Deathmatch" -or $Mode -eq "DM") {
    Write-Host "FFA dogfood: everyone hostile, no siege win. Connect 2+ clients."
}
Write-Host "Connect: .\scripts\Run-Game.ps1 -Server 127.0.0.1:$Port"
Write-Host "Args: $($argList -join ' ')"

if ($Wait) {
    & $exe @argList
    exit $LASTEXITCODE
}

Start-Process -FilePath $exe -ArgumentList $argList -WorkingDirectory $ProjectRoot | Out-Null
Write-Host "Server started (detached). Logs under Saved\Logs\"
