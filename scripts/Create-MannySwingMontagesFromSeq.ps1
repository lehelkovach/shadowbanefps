# Bake DefaultSlot montages for Steel B/C + Serath A/B/C from existing sequences.
# Prefer Editor closed (this script stops live Editor/Cmd processes first).
#
# Usage:
#   powershell -ExecutionPolicy Bypass -File .\scripts\Create-MannySwingMontagesFromSeq.ps1
param(
    [string]$EngineRoot = ""
)

$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "lib\UeCommon.ps1")

$ProjectRoot = Get-SBProjectRoot
$UProject = Get-SBUproject -ProjectRoot $ProjectRoot
$Engine = Resolve-SBEngineRoot -EngineRoot $EngineRoot
$EditorCmd = Get-SBEditorCmdExe -EngineRoot $Engine
$Py = Join-Path $PSScriptRoot "Create-MannySwingMontagesFromSeq.py"

Get-Process -Name "UnrealEditor","UnrealEditor-Cmd","ShadowbaneFPS","ShadowbaneFPSEditor" -ErrorAction SilentlyContinue |
    ForEach-Object {
        Write-Host "Stopping $($_.Name) ($($_.Id))..."
        Stop-Process -Id $_.Id -Force -ErrorAction SilentlyContinue
    }
Start-Sleep -Seconds 2

Write-Host "Creating Steel/Serath DefaultSlot montages via UnrealEditor-Cmd..."
& $EditorCmd $UProject `
    -ExecutePythonScript="$Py" `
    -unattended -nop4 -nosplash -NullRHI `
    -log
$Code = $LASTEXITCODE
Write-Host "Editor-Cmd exit: $Code"

$Combat = Join-Path $ProjectRoot "Content\Characters\Mannequins\Animations\Combat"
$Required = @(
    "AM_MM_SteelSwing_B.uasset",
    "AM_MM_SteelSwing_C.uasset",
    "AM_MM_SerathSwing_A.uasset",
    "AM_MM_SerathSwing_B.uasset",
    "AM_MM_SerathSwing_C.uasset"
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

$LogDir = Join-Path $ProjectRoot "Saved\Logs"
$Log = Get-ChildItem $LogDir -Filter "*.log" -ErrorAction SilentlyContinue |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First 1
if ($Log) {
    Write-Host "Log: $($Log.FullName)"
    Select-String -Path $Log.FullName -Pattern "Create-MannySwingMontagesFromSeq" -ErrorAction SilentlyContinue |
        Select-Object -Last 20 |
        ForEach-Object { Write-Host $_.Line }
}

if ($Missing.Count -gt 0) {
    Write-Host "WARN: missing after bake: $($Missing -join ', ') (exit $Code)."
    exit 1
}

Write-Host "Create-MannySwingMontagesFromSeq: all Steel B/C + Serath A/B/C montages present."
exit 0
