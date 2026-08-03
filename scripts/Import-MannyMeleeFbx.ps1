# Import Manny-retargeted melee FBX + bake DefaultSlot montages via UnrealEditor-Cmd.
# Prefer Editor closed (this script stops live Editor/Cmd processes first).
#
# Usage:
#   powershell -ExecutionPolicy Bypass -File .\scripts\Import-MannyMeleeFbx.ps1
# If Editor must stay open: Window → Execute Python Script → Import-MannyMeleeFbx.py
param(
    [string]$EngineRoot = ""
)

$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "lib\UeCommon.ps1")

$ProjectRoot = Get-SBProjectRoot
$UProject = Get-SBUproject -ProjectRoot $ProjectRoot
$Engine = Resolve-SBEngineRoot -EngineRoot $EngineRoot
$EditorCmd = Get-SBEditorCmdExe -EngineRoot $Engine
$Py = Join-Path $PSScriptRoot "Import-MannyMeleeFbx.py"

$FbxDir = Join-Path $ProjectRoot "Content\Characters\Mannequins\Animations\Combat\Import_MannyFbx"
if (-not (Test-Path $FbxDir)) {
    throw "Missing FBX staging folder: $FbxDir"
}
$FbxCount = @(Get-ChildItem $FbxDir -Filter "*.FBX" -ErrorAction SilentlyContinue).Count
Write-Host "FBX staging: $FbxDir ($FbxCount files)"

# Live Coding / Editor locks Content — close first.
Get-Process -Name "UnrealEditor","UnrealEditor-Cmd","ShadowbaneFPS","ShadowbaneFPSEditor" -ErrorAction SilentlyContinue |
    ForEach-Object {
        Write-Host "Stopping $($_.Name) ($($_.Id))..."
        Stop-Process -Id $_.Id -Force -ErrorAction SilentlyContinue
    }
Start-Sleep -Seconds 2

Write-Host "Importing Manny melee FBX + montages via UnrealEditor-Cmd..."
& $EditorCmd $UProject `
    -ExecutePythonScript="$Py" `
    -unattended -nop4 -nosplash -NullRHI `
    -log
$Code = $LASTEXITCODE
Write-Host "Editor-Cmd exit: $Code"

$Combat = Join-Path $ProjectRoot "Content\Characters\Mannequins\Animations\Combat"
$Required = @(
    "AS_MM_GreystoneSwing_A.uasset",
    "AM_MM_GreystoneSwing_A.uasset",
    "AS_MM_GreystoneSwing_B.uasset",
    "AM_MM_GreystoneSwing_B.uasset",
    "AS_MM_GreystoneSwing_C.uasset",
    "AM_MM_GreystoneSwing_C.uasset",
    "AS_MM_SteelSwing_A.uasset",
    "AM_MM_SteelSwing_A.uasset"
)

$Missing = @()
foreach ($Name in $Required) {
    $Path = Join-Path $Combat $Name
    if (Test-Path $Path) {
        Write-Host "OK: $Path"
    } else {
        Write-Host "MISSING: $Path"
        $Missing += $Name
    }
}

# Scrape latest log for PRIMARY_OK / errors.
$LogDir = Join-Path $ProjectRoot "Saved\Logs"
$Log = Get-ChildItem $LogDir -Filter "*.log" -ErrorAction SilentlyContinue |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First 1
if ($Log) {
    Write-Host "Log: $($Log.FullName)"
    Select-String -Path $Log.FullName -Pattern "Import-MannyMeleeFbx" -ErrorAction SilentlyContinue |
        Select-Object -Last 20 |
        ForEach-Object { Write-Host $_.Line }
}

if ($Missing.Count -gt 0) {
    Write-Host "WARN: missing after import: $($Missing -join ', ') (exit $Code)."
    Write-Host "If Editor was open elsewhere, close it and re-run, or use Window→Execute Python Script."
    exit 1
}

Write-Host "Import-MannyMeleeFbx: all required Greystone A/B/C + Steel A assets present."
exit 0
