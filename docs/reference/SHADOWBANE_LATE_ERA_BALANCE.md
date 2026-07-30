# Late official Shadowbane balance baseline (Ubisoft era)

Official retail Shadowbane shut down **2009-05-01** (Ubisoft). The last major
published live balance work before that is archived on the Morloch Wiki
(community mirror of official notes):

| Patch | Date | Role |
| --- | --- | --- |
| **21** | 2007-10-09 | Weapon power ATR removed; weapon power damage rebalanced; stealth/hide casting; many class power tweaks |
| **22 — Reboot** | 2008-03-25 | Full character/server reset; armor redistribute; bow/crossbow power damage down; snares cut; caster stealth capped; class armor sets; siege player-AoE on engines removed; wall archers removed |
| **23** | 2008-08-01 | Last major published balance pass — weapon skill ladder 25→110%; class staves/wands; glass weapon damage types; mage/channeler HP/level up; snares 11–60%; power-block immunity |

Sources:
- https://morloch.shadowbaneemulator.com/index.php/Patch_History
- https://morloch.shadowbaneemulator.com/index.php/Patch_22
- https://morloch.shadowbaneemulator.com/index.php/Patch_23

## Themes to carry into *Shadowbane FPS* balancing

These are **heuristics**, not a demand to reimplement 2008 math:

1. **Unfixable gear forced a reboot (P22)** — keep shop catalog small; log item ids; refuse snowballing unanswerable kits.
2. **Ranged/siege mage pressure was dialed** — bow/crossbow power % damage cut; our Channeler/Ranger structure + player DPS should be strong at objectives, not delete tanks free.
3. **Stealth is powerful but capped** — caster stealth PR caps / faster cast but limited (P22); Assassin should win openings, lose to detection + numbers.
4. **Snares are bands, not perma-roots** — P22/P23 snare retunes; Frost Wizard CC must be telegraphed and temporary (§10).
5. **Armor / weapon ladders matter** — P23 rebuilt weapon % tiers and class weapons; our 4 shop slots should create readable armor vs power matchups in `combat_*.csv`.
6. **Siege is not just PvP DPS** — P22 removed engine player AoE / wall archers; structure damage attribution stays first-class in telemetry.
7. **Healers sustain, don’t immortalize** — keep heal telemetry; if heal/damage ratio on contested objectives explodes, nerf aura before buffing everyone else’s DPS.

## How the balance agent should use this

1. Run `tools/balance/analyze_combat.py summary <TelemetryDir> --compare-late-sb`
2. Inspect build-vs-build, class-vs-class, power-vs-victim-class, heals, power-vs-armor
3. Propose nerfs/buffs with **CSV evidence** + which late-SB theme it echoes
4. Do **not** invent fake patch numbers; cite P21/P22/P23 or say “pilot-only”

Emulator (SBEmu) patches after 2014 are **out of scope** unless explicitly requested — the user asked for the last Ubisoft live era.
