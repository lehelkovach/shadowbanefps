# shadowbanefps

A Shadowbane-inspired **20-minute conquest siege** multiplayer pilot built in
Unreal Engine 5. One team attacks a fortified objective while the other defends;
both sides build a group from pre-built characters **without seeing the enemy
composition**, then adapt as the siege reveals what the enemy brought.

The pilot deliberately tests one thing — the match structure and the information
game — and leaves progression, economy, and persistence out of scope.

## Start here (Gracen / local Cursor Agent)

**Open and follow:** [`GRACEN_CURSOR_AGENT_INSTRUCTIONS.md`](./GRACEN_CURSOR_AGENT_INSTRUCTIONS.md)

Prompt for Cursor:

> Follow `GRACEN_CURSOR_AGENT_INSTRUCTIONS.md` and take over client + server development on the `dev` branch.

That covers Editor builds, PIE, automation, LinuxServer cook, and hot deploy to DEV.

## Live DEV server

| | |
| --- | --- |
| **Connect** | **`144.24.46.16:7777`** (UDP) |
| **SSH** | `ubuntu@144.24.46.16` |
| **Branch** | `dev` (hot deploy) → merge to `main` when stable |
| Docs | [`docs/DEV_WORKFLOW.md`](docs/DEV_WORKFLOW.md), [`docs/OCI_DEPLOY.md`](docs/OCI_DEPLOY.md) |
| Helpers | `scripts/Dev-Push.ps1`, `Setup-DevSsh.ps1`, `Cook-LinuxServer.ps1`, `Connect-DevServer.ps1` |

## Docs
- **[Game design](docs/game-design.md)** — full pilot design spec (Draft 1.0).
- **[Setup & operations](docs/SETUP.md)** — hardware roles, Epic/UE account,
  Windows build, Local Cursor Agent onboarding (§5), OCI dedicated-server plan.
- **[OCI dedicated server](docs/OCI_DEPLOY.md)** — live IP, Terraform, deploy,
  systemd, cook + client connect.
- **[DEV workflow](docs/DEV_WORKFLOW.md)** — `dev` branch + `Dev-Push.ps1` hot
  deploy (SSH, not OCI admin).
- **[Testing & logging](docs/TESTING.md)** — automation tests, log categories,
  telemetry CSV, headless run script.
- **[Placeholder art](docs/PLACEHOLDER_ART.md)** — dummy icons/runes/colors now;
  real-art swap list later.

## Status
**UE 5.5 Editor build verified on Gracen's machine.**  
**DEV dedicated-server VM is live** at `144.24.46.16:7777` (placeholder until
first LinuxServer cook is deployed). Engine target **5.5**.

Implemented (C++ / server-authoritative):
- Match flow, 20-min clock, phases, conquest stages, overtime, victory
- Shared character pawn with hitscan combat, heal/repair ticks, team colors
- 10 curated Shadowbane-flavored pre-built archetypes (runtime roster)
- Staged spawn points + post-death character switch (keys `1-0`, `R` respawn)
- Runtime **Broken Citadel** greybox (no `.umap` required yet)
- Destructible gate/breach, courtyard capture, final keep objective
- Debug HUD (timer, phase, stage, HP, roster)
- Logging (`LogShadowbane` / `Server` / `Client` / `Net` / `Telemetry` / `Combat`) + match telemetry CSV
- Automation: `ShadowbaneFPS.Rules|Server|Client|Integration|...` (see `docs/TESTING.md`)
- Placeholder art kit: team colors, role rune discs, HUD icon chips, world markers

See [`docs/SETUP.md`](docs/SETUP.md) to build/run on a Windows GPU machine.

## Project layout
```
ShadowbaneFPS.uproject     Unreal project (UE 5.5)
Config/                    Engine / game / input .ini defaults
Source/
  *.Target.cs              Game, Editor, and dedicated Server build targets
  ShadowbaneFPS/
    Core/                  GameMode, GameState, PlayerState, shared types
    Characters/            Pre-built character archetype data asset
    Siege/                 Destructible structures, capture point, objective
    Art/                   Placeholder icons / runes / world markers
docs/                      Design + setup + OCI deploy + testing docs
infra/oci/                 Terraform + systemd for dedicated-server VMs
scripts/deploy-server.sh   rsync + systemd restart (dev|release)
scripts/Dev-Push.ps1       commit-friendly cook + hot deploy to DEV
scripts/Setup-DevSsh.ps1   verify deploy SSH key → VM
scripts/Cook-LinuxServer.ps1   Windows cook helper
scripts/Connect-DevServer.ps1  client → 144.24.46.16:7777
scripts/RunAutomationTests.ps1 all ShadowbaneFPS.* headless
scripts/RunClientTests.ps1     ShadowbaneFPS.Client.*
scripts/RunServerTests.ps1     ShadowbaneFPS.Server.*
scripts/RunIntegrationTests.ps1 ShadowbaneFPS.Integration.*
.github/workflows/deploy-dev.yml  optional SSH deploy Action
.env.example               Required OCI/SSH env var NAMES (no values)
GRACEN_CURSOR_AGENT_INSTRUCTIONS.md   **Gracen takeover prompt (client+server)**
```
