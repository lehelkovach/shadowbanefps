# Simple DEV workflow - cook + hot deploy to OCI
#
# Recommended branch model (2 people, keep it dumb):
#   - `dev`   → continuous playtest server (auto/manual deploy target)
#   - `main`  → verified baseline (merge from `dev` when stable)
#
# Gracen does NOT need OCI admin. He needs an SSH deploy key the VM trusts.
# "Hot push" here means: new dedicated-server build + systemd restart
# (Unreal does not live-patch C++ into a running server).
#
# Usage (from repo root on Windows):
#   .\scripts\Dev-Push.ps1
#   .\scripts\Dev-Push.ps1 -SkipCook          # redeploy last Dist\Server\LinuxServer
#   .\scripts\Dev-Push.ps1 -SkipDeploy        # cook only
#   .\scripts\Dev-Push.ps1 -SkipGit           # don't pull/push
#   .\scripts\Dev-Push.ps1 -Config Shipping

param(
    [string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.5",
    [ValidateSet("Development", "Shipping")]
    [string]$Config = "Development",
    [string]$DevBranch = "dev",
    [string]$ServerHost = "144.24.46.16",
    [string]$SshUser = "ubuntu",
    [string]$SshIdentity = "",
    [switch]$SkipGit,
    [switch]$SkipCook,
    [switch]$SkipDeploy,
    [switch]$NoPush
)

$ErrorActionPreference = "Stop"
$ProjectRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
Set-Location $ProjectRoot

function Resolve-SshIdentity {
    param([string]$Explicit)
    if ($Explicit -and (Test-Path $Explicit)) { return $Explicit }
    if ($env:SB_SSH_IDENTITY -and (Test-Path $env:SB_SSH_IDENTITY)) { return $env:SB_SSH_IDENTITY }
    if ($env:SSH_IDENTITY_FILE -and (Test-Path $env:SSH_IDENTITY_FILE)) { return $env:SSH_IDENTITY_FILE }
    $candidates = @(
        "$env:USERPROFILE\.ssh\shadowbanefps_deploy",
        "$env:USERPROFILE\.ssh\shadowbanefps_oci",
        "$env:USERPROFILE\.ssh\id_ed25519",
        "$env:USERPROFILE\.ssh\id_rsa"
    )
    foreach ($c in $candidates) {
        if (Test-Path $c) { return $c }
    }
    return ""
}

Write-Host "=== ShadowbaneFPS Dev-Push ==="
Write-Host "Repo   : $ProjectRoot"
Write-Host "Branch : $DevBranch"
Write-Host "Server : ${SshUser}@${ServerHost}:7777"
Write-Host ""

# --- Git: stay on dev, pull latest, optional push of local commits ---
if (-not $SkipGit) {
    git fetch origin
    $current = (git rev-parse --abbrev-ref HEAD).Trim()
    if ($current -ne $DevBranch) {
        # Create local tracking branch if needed
        $remoteHasDev = git rev-parse --verify --quiet "origin/$DevBranch"
        if ($LASTEXITCODE -eq 0) {
            git checkout $DevBranch
            git pull origin $DevBranch
        } else {
            Write-Host "Remote branch '$DevBranch' not found yet - creating from current HEAD."
            git checkout -B $DevBranch
        }
    } else {
        git pull origin $DevBranch 2>$null
        if ($LASTEXITCODE -ne 0) {
            Write-Host "Pull skipped/failed (new branch or no upstream yet) - continuing."
        }
    }

    if (-not $NoPush) {
        $pending = git status --porcelain
        if ($pending) {
            Write-Host "Working tree has local changes. Commit them first (or pass -SkipGit)."
            Write-Host $pending
            throw "Uncommitted changes present"
        }
        git push -u origin $DevBranch
    }
}

# --- Cook Linux dedicated server ---
$LinuxServer = Join-Path $ProjectRoot "Dist\Server\LinuxServer"
if (-not $SkipCook) {
    Write-Host "`n=== Cooking LinuxServer ($Config) ==="
    & (Join-Path $PSScriptRoot "Cook-LinuxServer.ps1") -EngineRoot $EngineRoot -Config $Config
} else {
    Write-Host "Skipping cook (-SkipCook)"
}

if (-not (Test-Path $LinuxServer)) {
    throw "Missing $LinuxServer - cook first or drop a package there"
}

# --- Deploy via SSH (not OCI API) ---
if ($SkipDeploy) {
    Write-Host "Skipping deploy (-SkipDeploy). Package ready at $LinuxServer"
    exit 0
}

$identity = Resolve-SshIdentity -Explicit $SshIdentity
if (-not $identity) {
    Write-Host @"

No SSH private key found for deploy.
Gracen does not need OCI admin - only an SSH key the VM already trusts.

Ask Lehel to:
  1) Generate a deploy keypair (or add your Windows .pub to the VM)
  2) Put the private key at %USERPROFILE%\.ssh\shadowbanefps_deploy
  3) Re-run: .\scripts\Dev-Push.ps1 -SkipCook

Or set SB_SSH_IDENTITY to the key path.
"@
    throw "SSH identity required for deploy"
}

$env:SB_DEV_IP = $ServerHost
$env:SB_SSH_USER = $SshUser
$env:SB_SSH_IDENTITY = $identity

Write-Host "`n=== Deploying to DEV ${SshUser}@${ServerHost} ==="

# Prefer Git Bash / WSL for the bash deploy script on Windows.
$bash = $null
foreach ($c in @(
    "C:\Program Files\Git\bin\bash.exe",
    "C:\Program Files\Git\usr\bin\bash.exe",
    "$env:LOCALAPPDATA\Programs\Git\bin\bash.exe"
)) {
    if (Test-Path $c) { $bash = $c; break }
}

$deploySh = Join-Path $PSScriptRoot "deploy-server.sh"
# Convert Windows path to a form bash understands when using Git Bash
$srcForBash = ($LinuxServer -replace '\\', '/')
if ($srcForBash -match '^([A-Za-z]):/(.*)$') {
    $srcForBash = "/$($Matches[1].ToLower())/$($Matches[2])"
}

if ($bash) {
    $env:MSYS_NO_PATHCONV = "1"
    & $bash -lc "export SB_DEV_IP='$ServerHost' SB_SSH_USER='$SshUser' SB_SSH_IDENTITY='$($identity -replace '\\','/')'; ./scripts/deploy-server.sh --target dev --src '$srcForBash'"
    if ($LASTEXITCODE -ne 0) { throw "deploy-server.sh failed ($LASTEXITCODE)" }
} else {
    throw "Git Bash not found. Install Git for Windows, or run ./scripts/deploy-server.sh from WSL."
}

Write-Host ""
Write-Host "DEV deploy complete."
Write-Host "  Connect: .\scripts\Connect-DevServer.ps1"
Write-Host "  URL:     ${ServerHost}:7777"
Write-Host "  Logs:    ssh -i `"$identity`" ${SshUser}@${ServerHost} 'sudo journalctl -u shadowbanefps-server -f'"
