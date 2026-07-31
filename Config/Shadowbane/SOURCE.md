# Shadowbane creation data sources

Used by the FPS pilot character builder / calculator (`USBShadowbaneCreationDb`, `USBCharacterCalculator`) and match-shop stub (`USBMatchShopCatalog`).

## Primary structured data (this folder)

| File | Source |
| --- | --- |
| `races.json`, `base-classes.json`, `prestige-classes.json`, `disciplines.json`, `starting-traits.json`, `stat-runes.json`, `mastery-runes.json` | [tdehart/shadowbane-db](https://github.com/tdehart/shadowbane-db) (JSON race/class/rune dump) |
| `readme.md` | Same repo — creation rules (55 start points, base 35/100, etc.) |

## shadowbanefps calculator (reference / formulas)

Community Shadowbane emulator character-builder dumps (formulas / tables), kept as reference alongside structured JSON:

| File | Notes |
| --- | --- |
| `shadowbanefps-dbdata.js` | Calculator database (powers, traits, race tables) |
| `shadowbanefps-calc.js` | UI + creation/stat/discipline logic (Vampire allow-lists; Saetor removed from race list) |
| `shadowbanefps-injection-topics.js` | Small calculator injection helper |

Pilot scope: we use the **JSON** for eligibility + race bonuses, the **ability CSV** for renamed discipline powers shown in the builder, and these dumps for validation against community calculator behavior. Full trainer UI (every rune / power / multi-discipline pick) is deferred; the in-game builder covers race → base path → prestige → one discipline → auto-allocated stats → FPS vitals.

Attribution: Shadowbane IP belongs to its respective owners; community data used for emulator/pilot theorycraft.

Prestige class renames: see `CLASS_RENAMES.md`.

Discipline display renames (wiki → pilot names): see `DISCIPLINE_RENAMES.md`.

## Ability rename / import CSV (GameDev)

| File | Notes |
| --- | --- |
| `shadowbane_ability_import.csv` | Imported from `\\GULAG\GameDev` (A:). Maps original → new discipline/ability names with mechanic summaries, rename status, wiki URLs, and review notes. **Consumed by** `USBShadowbaneCreationDb` for the death-time builder discipline/ability list. |
| `shadowbane_discipline_ability_database.xlsx` | Companion GameDev workbook copied alongside the CSV when present. |

## Client ENGLISH string dumps (P0)

Raw client localization dumps copied into `Config/Shadowbane/shadowbanefps/`:

| Raw file | Notes |
| --- | --- |
| `DataStringENGLISH.txt` | UTF-16 LE. Sections: `Power:`, `PowerDescription:`, `Skill:`, `SkillDesc:`, `RaceClassDiscTalent:`, etc. |
| `ItemENGLISH.txt` | ASCII. Lines: `<id> "Name" <Gender> "!PREFIX! !ITEMNAME! !SUFFIX!"` (~7238 items). |
| `EffectsENGLISH.txt` | UTF-16 LE. `EffectPrefix:` / `EffectSuffix:` affix display names. |
| `Config.wpak` | Optional opaque pack copied for later; **not** decrypted here (Powers.cfg etc. deferred). |

### Extracted JSON (re-runnable)

| File | Approx. count | Consumer |
| --- | --- |
| `powers.json` | ~1583 (`id` / `name` / `description`) | `USBShadowbaneCreationDb` — builder ability lines use `PowerDescription` when the CSV ability name matches a power id/name (rarely linked in the dump; CSV mechanic summary remains the default). |
| `skills.json` | ~93 | `USBShadowbaneCreationDb` — count shown in builder debug; SkillDesc attached when present. |
| `items-catalog.json` | ~7238 | `USBMatchShopCatalog` — full load; HUD shows count + first few names. |
| `effect-affixes.json` | ~663 | `USBMatchShopCatalog` |
| `race-class-disc-labels.json` | ~133 | Reference labels (mostly overlaps races/classes already in JSON); not required at runtime. |

**Re-extract** (after refreshing files under `shadowbanefps/`):

```powershell
.\scripts\Extract-ShadowbaneFPSClientStrings.ps1
```

Optional: `-SourceDir` / `-OutDir` if paths differ. Do **not** re-copy `shadowbane_ability_import.csv` via this script (CSV is separate GameDev import).

## In-game usage

While **dead**: press **C** to open the shadowbanefps creation builder.
- `[` `]` race · `,` `.` base path · `-` `=` prestige · `;` `'` discipline · **Enter** confirm · **R** respawn
Confirm snaps to the closest pilot roster pre-built and overlays shadowbanefps-calculated FPS vitals (including selected discipline name in the build summary).

Builder also shows:
- Discipline ability lines (CSV mechanic summary, replaced by PowerDescription when linkable)
- `Powers DB loaded: N | Skills: M`
- `Shop catalog: N items (... affixes) e.g. …` (stub until a real shop UI exists)
