# shadowbanefps

A Shadowbane-inspired **20-minute conquest siege** multiplayer pilot built in
Unreal Engine 5. One team attacks a fortified objective while the other defends;
both sides build a group from pre-built characters **without seeing the enemy
composition**, then adapt as the siege reveals what the enemy brought.

The pilot deliberately tests one thing — the match structure and the information
game — and leaves progression, economy, and persistence out of scope.

## Start here (Gracen / local Cursor Agent)

**Open and follow:** [`GRACEN_CURSOR_AGENT_INSTRUCTIONS.md`](./GRACEN_CURSOR_AGENT_INSTRUCTIONS.md)

That prompt tells Cursor on a Windows UE 5.5 machine to build `ShadowbaneFPSEditor`,
smoke-test the greybox, and run automation.

## Docs
- **[Game design](docs/game-design.md)** — full pilot design spec (Draft 1.0).
- **[Setup & operations](docs/SETUP.md)** — hardware roles, Epic/UE account,
  Windows build, Local Cursor Agent onboarding (§5), OCI dedicated-server plan.
- **[OCI dedicated server](docs/OCI_DEPLOY.md)** — Terraform VCN/NSG/VMs,
  `deploy-server.sh`, systemd, ports, rollback, client `IP:7777` connect.
- **[Testing & logging](docs/TESTING.md)** — automation tests, log categories,
  telemetry CSV, headless run script.
- **[Placeholder art](docs/PLACEHOLDER_ART.md)** — dummy icons/runes/colors now;
  real-art swap list later.

## Status
**UE 5.5 Editor build verified on Gracen's machine.** Engine target **5.5**
(`ShadowbaneFPS.uproject`).

Implemented (C++ / server-authoritative):
- Match flow, 20-min clock, phases, conquest stages, overtime, victory
- Shared character pawn with hitscan combat, heal/repair ticks, team colors
- 10 curated Shadowbane-flavored pre-built archetypes (runtime roster)
- Staged spawn points + post-death character switch (keys `1-0`, `R` respawn)
- Runtime **Broken Citadel** greybox (no `.umap` required yet)
- Destructible gate/breach, courtyard capture, final keep objective
- Debug HUD (timer, phase, stage, HP, roster)
- Logging (`LogShadowbane*`) + match telemetry CSV (`Saved/Telemetry/`)
- Automation tests under `ShadowbaneFPS.*` (see `docs/TESTING.md`)
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
.env.example               Required OCI/SSH env var NAMES (no values)
GRACEN_CURSOR_AGENT_INSTRUCTIONS.md   Local Cursor Agent prompt
```
