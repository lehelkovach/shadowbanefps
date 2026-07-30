# Shared Unreal helpers for ShadowbaneFPS scripts.
# Dot-source from other scripts:  . (Join-Path $PSScriptRoot "lib\UeCommon.ps1")
#
# This file lives in scripts/lib/ — project root is two levels up.

$script:SB_LibDir = $PSScriptRoot
$script:SB_ScriptsDir = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$script:SB_ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$script:SB_DefaultEngineRoot = "C:\Program Files\Epic Games\UE_5.5"

function Get-SBProjectRoot {
    return $script:SB_ProjectRoot
}

function Get-SBScriptsDir {
    return $script:SB_ScriptsDir
}

function Get-SBUproject {
    param([string]$ProjectRoot = (Get-SBProjectRoot))
    $uproject = Join-Path $ProjectRoot "ShadowbaneFPS.uproject"
    if (-not (Test-Path $uproject)) {
        throw "Missing project file: $uproject"
    }
    return $uproject
}

function Resolve-SBEngineRoot {
    param([string]$EngineRoot = "")
    if ($EngineRoot -and (Test-Path $EngineRoot)) { return $EngineRoot }
    if ($env:UE_ROOT -and (Test-Path $env:UE_ROOT)) { return $env:UE_ROOT }
    if ($env:UE55_ROOT -and (Test-Path $env:UE55_ROOT)) { return $env:UE55_ROOT }
    if (Test-Path $script:SB_DefaultEngineRoot) { return $script:SB_DefaultEngineRoot }
    throw "UE 5.5 not found. Pass -EngineRoot or set UE_ROOT."
}

function Get-SBBuildBat {
    param([string]$EngineRoot)
    $bat = Join-Path $EngineRoot "Engine\Build\BatchFiles\Build.bat"
    if (-not (Test-Path $bat)) { throw "Build.bat not found at $bat" }
    return $bat
}

function Get-SBEditorExe {
    param([string]$EngineRoot)
    $exe = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor.exe"
    if (-not (Test-Path $exe)) { throw "UnrealEditor.exe not found at $exe" }
    return $exe
}

function Get-SBEditorCmdExe {
    param([string]$EngineRoot)
    $exe = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
    if (-not (Test-Path $exe)) { throw "UnrealEditor-Cmd.exe not found at $exe" }
    return $exe
}

function Get-SBDefaultLogCmds {
    param([switch]$VerboseLogs)
    if ($VerboseLogs) {
        return 'LogShadowbane Log,LogShadowbaneServer Log,LogShadowbaneClient Log,LogShadowbaneNet Verbose,LogShadowbaneCombat Verbose,LogShadowbaneTelemetry Log'
    }
    return 'LogShadowbane Log,LogShadowbaneServer Log,LogShadowbaneClient Log,LogShadowbaneNet Log'
}

function Invoke-SBUbtBuild {
    param(
        [Parameter(Mandatory = $true)][string]$Target,
        [Parameter(Mandatory = $true)][string]$EngineRoot,
        [Parameter(Mandatory = $true)][string]$UProject,
        [ValidateSet("Development", "DebugGame", "Shipping")]
        [string]$Config = "Development",
        [string]$Platform = "Win64",
        [switch]$GenerateProjectFiles
    )

    $BuildBat = Get-SBBuildBat -EngineRoot $EngineRoot

    if ($GenerateProjectFiles) {
        Write-Host "Generating project files..."
        & $BuildBat -projectfiles "-project=$UProject" -game -engine -progress
        if ($LASTEXITCODE -ne 0) {
            throw "GenerateProjectFiles failed with exit code $LASTEXITCODE"
        }
    }

    Write-Host "Building $Target $Platform $Config ..."
    & $BuildBat $Target $Platform $Config "-Project=$UProject" -WaitMutex
    if ($LASTEXITCODE -ne 0) {
        throw "Build $Target failed with exit code $LASTEXITCODE"
    }
    Write-Host "Build OK: $Target ($Config|$Platform)"
}
