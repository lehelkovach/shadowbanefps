# Local FFA deathmatch dogfood: open lobby, everyone hostile, builder + nametags.
# Listen-server style Editor -game with ?Mode=FFA (optional bots).
#
# Usage:
#   .\scripts\Run-LocalFFA.ps1
#   .\scripts\Run-LocalFFA.ps1 -Bots 4
#   .\scripts\Run-LocalFFA.ps1 -Build

param(
    [int]$Bots = 0,
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

Write-Host "Launching local FFA deathmatch (Mode=FFA, Bots=$Bots)..."
& (Join-Path $PSScriptRoot "Run-Game.ps1") @buildArg -Bots $Bots -Mode FFA -VerboseLogs -ExtraArgs $Extra
