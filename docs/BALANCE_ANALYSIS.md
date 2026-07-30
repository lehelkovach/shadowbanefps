# Combat balance analysis

Pilot combat telemetry lands in `Saved/Telemetry/combat_*.csv`. Use the query
tool + balance agent to nerf/buff from **build vs build**, **class vs class**,
and **power vs armor/buff** evidence, compared against late Ubisoft Shadowbane
(Patches 22–23 → shutdown 2009).

## Quick start

```powershell
# After a playtest on Gracen's machine
.\scripts\Analyze-CombatBalance.ps1 -Command summary -CompareLateSB
.\scripts\Analyze-CombatBalance.ps1 -Command build-vs-build
.\scripts\Analyze-CombatBalance.ps1 -Command class-vs-class
.\scripts\Analyze-CombatBalance.ps1 -Command power-vs-victim-class
.\scripts\Analyze-CombatBalance.ps1 -Command power-vs-armor
.\scripts\Analyze-CombatBalance.ps1 -Command heals
```

Or:

```bash
python3 tools/balance/analyze_combat.py summary Saved/Telemetry --compare-late-sb
```

## Agent

Paste into Cursor:

> Follow `BALANCE_AGENT_INSTRUCTIONS.md` — analyze Saved/Telemetry combat CSVs and recommend nerfs/buffs vs late Ubisoft Shadowbane balance themes.

## References

- Design telemetry contract: `docs/game-design.md` §12
- Late SB baseline: `docs/reference/SHADOWBANE_LATE_ERA_BALANCE.md`
- Roster join metadata: `tools/balance/roster.json`
