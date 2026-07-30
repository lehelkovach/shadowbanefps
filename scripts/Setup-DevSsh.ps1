# Quick check that the DEV SSH deploy key is present and can reach the VM.
# Usage:
#   .\scripts\Setup-DevSsh.ps1
#   .\scripts\Setup-DevSsh.ps1 -Identity "$env:USERPROFILE\.ssh\shadowbanefps_deploy"

param(
    [string]$Identity = "",
    [string]$ServerHost = "144.24.46.16",
    [string]$SshUser = "ubuntu"
)

$ErrorActionPreference = "Stop"

function Resolve-Identity([string]$Explicit) {
    if ($Explicit -and (Test-Path $Explicit)) { return (Resolve-Path $Explicit).Path }
    foreach ($c in @(
        $env:SB_SSH_IDENTITY,
        $env:SSH_IDENTITY_FILE,
        "$env:USERPROFILE\.ssh\shadowbanefps_deploy",
        "$env:USERPROFILE\.ssh\shadowbanefps_oci",
        "$env:USERPROFILE\.ssh\id_ed25519",
        "$env:USERPROFILE\.ssh\id_rsa"
    )) {
        if ($c -and (Test-Path $c)) { return (Resolve-Path $c).Path }
    }
    return $null
}

$key = Resolve-Identity $Identity
if (-not $key) {
    Write-Host @"
No SSH private key found.

Ask Lehel for a deploy key and save it as:
  $env:USERPROFILE\.ssh\shadowbanefps_deploy

Public key must already be in ubuntu@$ServerHost ~/.ssh/authorized_keys
You do NOT need OCI admin credentials for this.
"@
    exit 1
}

Write-Host "Using key: $key"
Write-Host "Testing SSH ${SshUser}@${ServerHost} ..."

ssh -i $key -o StrictHostKeyChecking=accept-new -o BatchMode=yes -o ConnectTimeout=15 `
    "${SshUser}@${ServerHost}" "echo OK; hostname; sudo systemctl is-enabled shadowbanefps-server 2>/dev/null || true; sudo systemctl is-active shadowbanefps-server 2>/dev/null || true"

if ($LASTEXITCODE -ne 0) {
    throw "SSH failed. Key may not be authorized on the VM yet."
}

Write-Host ""
Write-Host "SSH OK. You can run: .\scripts\Dev-Push.ps1"
Write-Host "Client connect: .\scripts\Connect-DevServer.ps1  ($ServerHost:7777)"
