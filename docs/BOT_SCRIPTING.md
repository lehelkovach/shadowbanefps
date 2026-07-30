# Bot scripting (`.sbbot`)

Each bot loads a short **rule script** that decides what to chase and whether to fire.
Scripts live in `Config/BotScripts/<Id>.sbbot`. The bot’s archetype id is the script id
(e.g. `Warrior_Blade` → `Warrior_Blade.sbbot`). Missing files fall back to
`Default.sbbot`, then a built-in default.

Bots are still test fixtures — scripts make role flavor cheap to iterate without
recompiling.

## Markup cheat-sheet

```
# comment
id: Warrior_Blade
role: fighter
retarget: 1.0          # seconds between re-evaluations
engage: 2800           # max fire distance (uu)
fire: 0.55             # seconds between BotFire()

when <target> [in_range N] [qualifier] -> <action>
```

### Targets

| Token | Meaning |
| --- | --- |
| `enemy` | Nearest living hostile pawn |
| `ally_hurt` / `ally` | Nearest ally under ~65% HP |
| `capture` / `cap` | First uncaptured capture zone |
| `objective` / `obj` | Final conquest objective |
| `structure` / `gate` / `wall` | First non-destroyed structure |
| `else` / `default` | Always matches (put last) |

### Qualifiers (optional, informational for now)

`open`, `uncaptured`, `intact`, `unlocked`, `any`

### Actions

| Token | Behavior |
| --- | --- |
| `pursue` / `go` / `move` | Move toward target |
| `pursue fire` / `pursue_fire` / `engage` | Move + fire when in engage range |
| `fire` / `shoot` / `attack` | Fire (still faces/moves to target) |
| `hold` / `stay` | Stand still |
| `retreat` / `fallback` | Move away from target (kite) |

**First matching rule wins.** Order matters.

## Example

```
id: Healer_Prelate
role: healer
retarget: 1.0
engage: 1800
fire: 0.50

when ally_hurt in_range 3500 -> pursue
when enemy in_range 1800 -> pursue fire
when capture open -> pursue
when objective unlocked -> pursue
when else -> hold
```

## Code map

| Piece | Path |
| --- | --- |
| Parser / loader / eval | `Source/ShadowbaneFPS/AI/SBBotScript.*` |
| Bot runtime | `Source/ShadowbaneFPS/AI/SBBotController.*` |
| Scripts on disk | `Config/BotScripts/*.sbbot` |
| Tests | `ShadowbaneFPS.Bots.ScriptParse` |

Edit a `.sbbot`, restart the match (or respawn bots) — scripts load on bot configure / possess.
