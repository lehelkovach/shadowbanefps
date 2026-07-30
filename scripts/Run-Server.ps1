# Launch a local Win64 dedicated server for LAN / solo testing.
#
# Usage:
#   .\scripts\Run-Server.ps1
#   .\scripts\Run-Server.ps1 -Port 7777 -Build
#   .\scripts\Run-Server.ps1 -VerboseLogs
#
# Prefers Binaries\Win64\ShadowbaneFPSServer.exe; falls back to Editor -server.
# Then connect with:  .\scripts\Run-Game.ps1 -Server 127.0.0.1:7777

param(
    [string]$EngineRoot = "",
    [int]$Port = 7777,
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
    & (Join-Path $PSScriptRoot "Build.ps1") -Target Server -Config $Config -EngineRoot $Engine
}

$LogCmds = Get-SBDefaultLogCmds -VerboseLogs:$VerboseLogs
$ServerExe = Join-Path $ProjectRoot "Binaries\Win64\ShadowbaneFPSServer.exe"

$argList = @()
$exe = $null

if (Test-Path $ServerExe) {
    $exe = $ServerExe
    Write-Host "Using dedicated server binary: $ServerExe"
    $argList += @(
        "-log",
        "-PORT=$Port",
        "-LogCmds=$LogCmds"
    )
}
else {
    $exe = Get-SBEditorExe -EngineRoot $Engine
    Write-Host "No Binaries\Win64\ShadowbaneFPSServer.exe — launching Editor -server"
    $argList += @(
        $UProject,
        "-server",
        "-log",
        "-PORT=$Port",
        "-LogCmds=$LogCmds"
    )
}

if ($ExtraArgs) { $argList += $ExtraArgs }

Write-Host "Server listening on UDP $Port"
Write-Host "Connect: .\scripts\Run-Game.ps1 -Server 127.0.0.1:$Port"
Write-Host "Args: $($argList -join ' ')"

if ($Wait) {
    & $exe @argList
    exit $LASTEXITCODE
}

Start-Process -FilePath $exe -ArgumentList $argList -WorkingDirectory $ProjectRoot | Out-Null
Write-Host "Server started (detached). Logs under Saved\Logs\"
