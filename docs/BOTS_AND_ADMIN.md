# Bots + Admin Spectator (pilot populate testing)

We do not have a full player base yet. The pilot therefore supports **AI bots** that
fill attacker/defender slots and an **admin spectator client** so you can watch a
match play out and collect combat telemetry.

Design pointer: `docs/game-design.md` §12.1.

## Goals

| Piece | Purpose |
| --- | --- |
| Basic AI bots | Populate ~5v5 with Shadowbane archetypes; move toward capture/objective/enemies; fire |
| Admin client | Join as free-cam spectator, spawn bots, watch siege + HUD + logs |
| Telemetry | Same `combat_*.csv` path so balance agent works without real humans |

## Launch (Windows)

```powershell
# Spectate a bot-filled match (listen server / PIE-style -game)
.\scripts\Run-AdminClient.ps1 -Bots 8

# Or manually
.\scripts\Run-Editor.ps1 -ExtraArgs "?Bots=8?AdminSpectate=1"
.\scripts\Run-Game.ps1 -ExtraArgs "?Bots=10?AdminSpectate=1"
```

Console (while playing / spectating with authority):

```
AddBots 8
AdminSpectate
```

## Behavior (v1 — intentionally dumb)

- `ASBBotController` picks a legal roster archetype for its team
- Retargets every ~1.25s: nearest enemy, else capture/objective/structure by side
- Steers with `AddMovementInput` (no NavMesh required for greybox)
- Fires hitscan via `ASBCharacter::BotFire`

Not in v1: behavior trees, cover, shop buys, coordinated pushes, voice. Improve after first spectate sessions.

## Dev plan / next iterations

1. **Ship v1** — bots + admin spectate (this scaffold)  
2. **PIE soak** — 8–10 bots, collect `Saved/Telemetry/combat_*.csv`  
3. **Balance agent** — `BALANCE_AGENT_INSTRUCTIONS.md` on those CSVs  
4. **Smarter bots** — capture weight, heal/repair roles, shop buys  
5. **Dedicated admin** — optional second process: dedicated server with bots + thin spectator client connecting to DEV IP  
6. **Recording** — demo/rec for async review  

## Code map

| File | Role |
| --- | --- |
| `Source/ShadowbaneFPS/AI/SBBotController.*` | Bot AI |
| `ASBSiegeGameMode::SpawnBots` / `?Bots=` | Populate |
| `ASBPlayerController::EnterAdminSpectate` / `AddBots` | Admin |
| `USBRulesLibrary::SplitBotsAcrossTeams` | 5v5 fill helper |
| `scripts/Run-AdminClient.ps1` | One-click spectator launch |
