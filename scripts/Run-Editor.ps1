# Launch Unreal Editor for ShadowbaneFPS (PIE / content work).
#
# Usage:
#   .\scripts\Run-Editor.ps1                 # defaults to -dx11 (NVIDIA D3D12 crash workaround)
#   .\scripts\Run-Editor.ps1 -RHI Dx12       # force D3D12 if needed
#   .\scripts\Run-Editor.ps1 -RHI Default    # engine default RHI
#   .\scripts\Run-Editor.ps1 -VerboseLogs
#   .\scripts\Run-Editor.ps1 -Build
#   .\scripts\Run-Editor.ps1 -Wait   # block until editor exits

param(
    [string]$EngineRoot = "",
    [ValidateSet("Default", "Dx11", "Dx12", "Vulkan")]
    [string]$RHI = "Dx11",
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
$argList = [System.Collections.Generic.List[string]]::new()
$argList.Add($UProject)
$argList.Add("-log")
$argList.Add("-LogCmds=$LogCmds")
foreach ($dbg in (Get-SBDebugArgs -VerboseLogs:$VerboseLogs)) {
    $argList.Add($dbg)
}

switch ($RHI) {
    "Dx11"   { $argList.Add("-dx11"); Write-Host "RHI: DirectX 11" }
    "Dx12"   { $argList.Add("-d3d12"); Write-Host "RHI: DirectX 12" }
    "Vulkan" { $argList.Add("-vulkan"); Write-Host "RHI: Vulkan" }
}

if ($ExtraArgs) {
    foreach ($a in ($ExtraArgs -split '\s+' | Where-Object { $_ })) {
        $argList.Add($a)
    }
}

if ($Wait) {
    & $Editor @($argList.ToArray())
    exit $LASTEXITCODE
}

Start-Process -FilePath $Editor -ArgumentList $argList.ToArray() -WorkingDirectory $ProjectRoot | Out-Null
Write-Host "Editor started (detached). Attach VS via .\scripts\Open-VS.ps1 then Debug -> Attach to Process -> UnrealEditor.exe"

