# shadowbanefps

A Shadowbane-inspired **20-minute conquest siege** multiplayer pilot built in
Unreal Engine 5. One team attacks a fortified objective while the other defends;
both sides build a group from pre-built characters **without seeing the enemy
composition**, then adapt as the siege reveals what the enemy brought.

The pilot deliberately tests one thing — the match structure and the information
game — and leaves progression, economy, and persistence out of scope.

## Docs
- **[Game design](docs/game-design.md)** — full pilot design spec (Draft 1.0).
- **[Setup & operations](docs/SETUP.md)** — hardware, Epic/UE account, building on
  Windows, and running the dedicated server on an OCI VM.

## Status
Early scaffold. Engine target **UE 5.5** (`ShadowbaneFPS.uproject`).

Implemented (C++ framework, server-authoritative):
- Match flow, 20-min clock, phase pacing, conquest stages, overtime, victory
  (`SBSiegeGameMode` / `SBSiegeGameState`).
- Teams, selected pre-built character, respawn + post-death switching
  (`SBPlayerState`, `SBCharacterArchetype`).
- Siege actors: destructible structures, courtyard capture point, inner-keep
  final objective.

See [`docs/SETUP.md`](docs/SETUP.md) for what's next and how to build/run.

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
