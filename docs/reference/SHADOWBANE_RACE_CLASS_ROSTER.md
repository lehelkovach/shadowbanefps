# Shadowbane race / class roster (Morloch Wiki → pilot)

Source: [Morloch Wiki — Character Creation](https://morloch.shadowbaneemulator.com/index.php/Character_Creation),
[Base Class](https://morloch.shadowbaneemulator.com/index.php/Base_Class),
race pages (High Elf, Nightshades People, Minotaur People, Elves, etc.).

This is the **authoritative lore matrix** for the FPS pilot. Runtime data lives in
`Source/ShadowbaneFPS/Characters/SBPilotRoster.cpp`. Keep both in sync.

Pilot rule: curated **pre-builts** only (not freeform trainers). ~8–12 builds.
Full Shadowbane trait/discipline sprawl is deferred to §16.

---

## Four Paths (base class)

| Path | SB name | Pilot FPS role |
| --- | --- | --- |
| Fighter | Path of Might | Melee / bow DPS, tanks, siege muscle |
| Healer | Path of Faith | Heal / support / holy-fire hybrids |
| Mage | Path of Lore | Bolt / CC / siege burn casters |
| Rogue | Path of Skill | Stealth, detection, sabotage, bow/dagger |

Prestige (promotion) is chosen at creation for the pilot — there is no level-10 promote step.

---

## Races (pilot-relevant)

| Race | Cost* | Lore silhouette | Innate (compressed for FPS) | Pilot builds |
| --- | --- | --- | --- | --- |
| **Human** | 0 | Average height, adaptable | Broad skill coverage; only Inquisitors (not in pilot) | Ironbrand, Ashwake, Dawnward, Wallwarden |
| **High Elf** | 5 | Pale gold, pointed ears, muscular hybrid | Snare/root break (Wildkin's Chase) | Nightcoil Assassin |
| **Nightshades** | 10 | Corpse-grey, hairless, black eyes, whisper | Innate stealth / detection; void resists | Underlane Scout |
| **Elf** | 15 | Tall, lithe, pointed ears | Magic resist flavor; strong mana; heal bonus | Greyfen Warden, Rimebind Wizard |
| **Dwarf** | 15 | Short, stout, stone-forged | Physical toughness; high stamina | Bastion Templar, Breachwright |
| **Minotaur** | 15 | Bull head + horns, cloven legs, hulking | Stun resist flavor; highest stam/HP; low mana | Horned Gatebreaker |

\*Creation cost on Morloch (attribute points). Informational only in the pilot.

**Not in pilot yet:** Accipitridae, Centaur, Half-Giant, Badawian, Shedim, Vampire.

---

## Prestige classes used in pilot

| Prestige | Base path(s) | Pilot mapping |
| --- | --- | --- |
| Warrior | Fighter | Ironbrand (Blade Weaving), Wallwarden (Fortress) |
| Huntress | Fighter/Rogue | Greyfen Warden (Way of the Bow) |
| Nightstalker | Rogue | Nightcoil Assassin (Shadowmantle) — SB prestige hunts unholy; we use as stealth assassin |
| Furia / Spellweaver | Mage/Healer | Ashwake Spellweaver (Flame) — Ravager-like fire siege burn |
| Smite Cleric | Healer | Dawnward Smite Cleric (Blessed Mantle) |
| Warlock | Fighter/Mage | Rimebind Wizard uses **Warlock** promotion label with **Frost** discipline (CC mage) |
| Thief / Scout | Rogue | Underlane Scout (Silent Step) — Scout detection + Thief sabotage |
| Templar / Paladin | Fighter/Healer | Bastion Templar (Bulwark) |
| Huntmaster | Fighter | Breachwright Engineer (Siegecraft) |
| Warlord | Fighter | Horned Gatebreaker Minotaur (Blade Weaving) |

---

## Curated pilot roster (11)

| Id | Display | Race | Base | Class | Promotion | Discipline | Signature | Side | Dup |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `Warrior_Blade` | Ironbrand Warrior | Human | Fighter | Warrior | Warlord | Blade Weaving | heavy melee pressure | Both | 2 |
| `Ranger_Scout` | Greyfen Warden | Elf | Rogue/Fighter | Warden | Huntress | Way of the Bow | long-range bow / high mobility | Both | 2 |
| `Assassin_Shadow` | Nightcoil Assassin | High Elf | Rogue | Assassin | Nightstalker | Shadowmantle | stealth / close burst / snare-break | Both | 1 |
| `Channeler_Flame` | Ashwake Spellweaver | Human | Mage | Spellweaver | Furia | Flame | fire magic / siege burn | Both | 1 |
| `Healer_Prelate` | Dawnward Smite Cleric | Human | Healer | Smite Cleric | Smite Cleric | Blessed Mantle | sustained healing aura | Both | 2 |
| `Wizard_Frost` | Rimebind Wizard | Elf | Mage | Wizard | Warlock | Frost | crowd control / chill zones | Both | 1 |
| `Scout_Thief` | Underlane Scout | Nightshades | Rogue | Thief | Saboteur | Silent Step | scouting / detection / sabotage | Both | 2 |
| `Templar_Bulwark` | Bastion Templar | Dwarf | Fighter | Templar | Paladin | Bulwark | frontline hold / light heal | Both | 2 |
| `Siege_Engineer` | Breachwright Engineer | Dwarf | Fighter | Warrior | Huntmaster | Siegecraft | siege device / structure damage | Attackers | 1 |
| `Defender_Warden` | Wallwarden | Human | Fighter | Warrior | Warlord | Fortress | repair / emplacement defense | Defenders | 2 |
| `Minotaur_Bulwark` | Horned Gatebreaker | Minotaur | Fighter | Warrior | Warlord | Blade Weaving | minotaur melee bruiser / gate pressure | Both | 1 |

Coverage: melee, ranged, stealth, fire siege mage, healer, frost CC, detection/scout, tank/support, attacker siege, defender repair, distinct Minotaur silhouette.

---

## Vitals biases (pilot FPS)

Constitution → HP/stamina; Spirit/Int → mana (Morloch attribute mapping, compressed).

| Race | MaxHP bias | MaxMana | MaxStamina | Notes |
| --- | --- | --- | --- | --- |
| Human | mid | 100 | 100 | Baseline |
| Elf | low-mid | 120 | 90 | Mage/ranger mana |
| High Elf | mid | 95 | 110 | Wildkin stam for snare-break |
| Nightshades | low | 100 | 105 | Fragile, sneaky |
| Dwarf | high | 80 | 120 | Stone endurance |
| Minotaur | highest | 60 | 130 | Brute stam; weak spirit |

Per-build numbers are set in `SBPilotRoster.cpp`.

---

## Changelog

- 2026-07-31: Initial matrix from Morloch Character Creation + race pages; restored Wallwarden; kept Minotaur; added base path + vitals.
