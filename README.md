# shadowbanefps

A Shadowbane-inspired **20-minute conquest siege** multiplayer pilot built in
Unreal Engine 5. One team attacks a fortified objective while the other defends;
both sides build a group from pre-built characters **without seeing the enemy
composition**, then adapt as the siege reveals what the enemy brought.

The pilot deliberately tests one thing — the match structure and the information
game — and leaves progression, economy, and persistence out of scope.

## Docs
- **[Game design](docs/game-design.md)** — full pilot design spec (Draft 1.0).
- **[Setup & operations](docs/SETUP.md)** — hardware roles, Epic/UE account,
  Windows build, **friend Local Cursor Agent onboarding** (§5), and OCI
  dedicated-server plan.
- **[Testing & logging](docs/TESTING.md)** — automation tests, log categories,
  telemetry CSV, headless run script.

## Status
Playable greybox pilot in progress. Engine target **UE 5.5** (`ShadowbaneFPS.uproject`).

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
docs/                      Design doc + setup/ops guide
```
