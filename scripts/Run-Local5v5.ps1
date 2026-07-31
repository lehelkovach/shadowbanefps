# Local 5v5 playtest: you (hero) + 4 attacker bots vs 5 defender bots.
# Starts listen-server style Editor -game with Bots=9 (auto-splits 4 atk / 5 def).

param(
    [ValidateSet("Default", "Dx11", "Dx12", "Vulkan")]
    [string]$RHI = "Dx11",
    [switch]$Build
)

$ErrorActionPreference = "Stop"
$Extra = "-windowed -ResX=1600 -ResY=900"
switch ($RHI) {
    "Dx11"   { $Extra += " -dx11" }
    "Dx12"   { $Extra += " -d3d12" }
    "Vulkan" { $Extra += " -vulkan" }
}

$buildArg = @{}
if ($Build) { $buildArg["Build"] = $true }

Write-Host "Launching local 5v5 (hero on Attackers, Bots=9 -> 4 atk + 5 def)..."
& (Join-Path $PSScriptRoot "Run-Game.ps1") @buildArg -Bots 9 -VerboseLogs -ExtraArgs $Extra
