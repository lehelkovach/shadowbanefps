# FFA open-lobby dogfood (no matchmaker)

Simplest multiplayer path for testing: connect to an open dedicated server,
pick a hero name + race/class build, fight everyone (free-for-all).

**Not in scope:** login/auth, server browser, LoL queue / matchmaker
(see `sb-login-match-funnel` canvas — Phase-later only).

## URL option

| Option | Effect |
| --- | --- |
| `?Mode=FFA` (or `Deathmatch` / `DM`) | Everyone hostile; siege win disabled; respawn + builder still work |
| `?Bots=N` | Fill with bots (all hostile in FFA) |

## Local two-player FFA

```powershell
# Terminal A — dedicated with FFA
.\scripts\Run-Server.ps1 -Mode FFA -Port 7777

# Terminal B — player 1
.\scripts\Run-Game.ps1 -Server 127.0.0.1:7777 -ExtraArgs "-dx11"

# Terminal C — player 2 (second machine or same box)
.\scripts\Run-Game.ps1 -Server 127.0.0.1:7777 -ExtraArgs "-dx11"
```

Optional bots on the server:

```powershell
.\scripts\Run-Server.ps1 -Mode FFA -Bots 4
```

Offline smoke (single process, Editor `-game`):

```powershell
.\scripts\Run-Game.ps1 -Mode FFA -Bots 3 -ExtraArgs "-dx11"
```

## DEV server (OCI `144.24.46.16:7777`)

1. Deploy a build whose dedicated is launched with `?Mode=FFA` (systemd / cook args).
2. Each player:

```powershell
.\scripts\Connect-DevServer.ps1
# or
.\scripts\Run-Game.ps1 -Server 144.24.46.16:7777 -ExtraArgs "-dx11"
```

No queue — join the open lobby anytime.

## Name + build

While **dead** (or after death overlay):

1. Press **C** — creation builder
2. Press **N** — type hero name (Backspace / Enter to lock)
   - Or console: `SBName MyHero`
3. Cycle race / path / prestige / discipline (`[ ]` `, .` `- =` `; '`)
4. **Enter** — confirm → `PlayerState::SetPlayerName` + vitals overlay
5. Respawn (**R** / auto) — nametag floats over your head

## HUD

- `MODE FFA DEATHMATCH` when `MatchMode=FreeForAll`
- Local `You: Name | K/D k/d`
- World nametags (`Name` + K/D for others)
- Siege objective chrome hidden in FFA

## Friendly fire

In FFA, `USBRulesLibrary::IsFriendlyFire(..., bFreeForAll=true)` never blocks.
Heals only apply to self (no team heals).
