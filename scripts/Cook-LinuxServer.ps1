# Cook UE 5.5 Linux dedicated server (x86_64) for OCI deploy.
# Requires: UE 5.5 + Linux cross-compile toolchain (LINUX_MULTIARCH_ROOT).
#
# Usage:
#   .\scripts\Cook-LinuxServer.ps1
#   .\scripts\Cook-LinuxServer.ps1 -EngineRoot "D:\UE\UE_5.5" -Config Development

param(
    [string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.5",
    [ValidateSet("Development", "Shipping")]
    [string]$Config = "Development",
    [string]$ArchiveDir = ""
)

$ErrorActionPreference = "Stop"
$ProjectRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$UProject = Join-Path $ProjectRoot "ShadowbaneFPS.uproject"
$RunUAT = Join-Path $EngineRoot "Engine\Build\BatchFiles\RunUAT.bat"

if (-not $ArchiveDir) {
    $ArchiveDir = Join-Path $ProjectRoot "Dist\Server"
}

if (-not (Test-Path $UProject)) { throw "Missing $UProject" }
if (-not (Test-Path $RunUAT)) {
    throw "RunUAT not found at $RunUAT — install UE 5.5 and pass -EngineRoot"
}

Write-Host "Project : $UProject"
Write-Host "Engine  : $EngineRoot"
Write-Host "Config  : $Config"
Write-Host "Archive : $ArchiveDir"
Write-Host ""

New-Item -ItemType Directory -Force -Path $ArchiveDir | Out-Null

& $RunUAT BuildCookRun `
    "-project=$UProject" `
    -noP4 `
    -platform=Linux `
    "-serverconfig=$Config" `
    -server `
    -noclient `
    -cook `
    -stage `
    -pak `
    -archive `
    "-archivedirectory=$ArchiveDir"

if ($LASTEXITCODE -ne 0) {
    throw "Cook failed with exit code $LASTEXITCODE"
}

$LinuxServer = Join-Path $ArchiveDir "LinuxServer"
Write-Host ""
Write-Host "Cook finished."
Write-Host "  Package: $LinuxServer"
Write-Host "  Deploy:  ./scripts/deploy-server.sh --target dev --src Dist/Server/LinuxServer"
Write-Host "  Connect: .\scripts\Connect-DevServer.ps1   # 144.24.46.16:7777"
