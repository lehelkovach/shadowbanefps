# Bake AM_MM_AxeSwing_01 via UnrealEditor-Cmd + Python.
param(
    [string]$EngineRoot = ""
)

$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "lib\UeCommon.ps1")

$ProjectRoot = Get-SBProjectRoot
$UProject = Get-SBUproject -ProjectRoot $ProjectRoot
$Engine = Resolve-SBEngineRoot -EngineRoot $EngineRoot
$EditorCmd = Get-SBEditorCmdExe -EngineRoot $Engine
$Py = Join-Path $PSScriptRoot "Create-MeleeSwingMontage.py"

# Live Coding / Editor locks Content — close first.
Get-Process -Name "UnrealEditor","UnrealEditor-Cmd","ShadowbaneFPS","ShadowbaneFPSEditor" -ErrorAction SilentlyContinue |
    ForEach-Object {
        Write-Host "Stopping $($_.Name) ($($_.Id))..."
        Stop-Process -Id $_.Id -Force -ErrorAction SilentlyContinue
    }
Start-Sleep -Seconds 2

Write-Host "Baking melee swing montage via UnrealEditor-Cmd..."
& $EditorCmd $UProject `
    -ExecutePythonScript="$Py" `
    -unattended -nop4 -nosplash -NullRHI `
    -log
$Code = $LASTEXITCODE
Write-Host "Editor-Cmd exit: $Code"

$Montage = Join-Path $ProjectRoot "Content\Characters\Mannequins\Animations\Combat\AM_MM_AxeSwing_01.uasset"
$Seq = Join-Path $ProjectRoot "Content\Characters\Mannequins\Animations\Combat\AS_MM_AxeSwing_01.uasset"
if ((Test-Path $Montage) -and (Test-Path $Seq)) {
    Write-Host "OK: $Montage"
    Write-Host "OK: $Seq"
    exit 0
}

Write-Host "WARN: montage assets missing after bake (exit $Code)."
exit 1
