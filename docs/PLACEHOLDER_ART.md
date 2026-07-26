# Placeholder Art (Dummy Icons / Runes / Colors)

For the pilot: **use dummy graphics**. Do not block gameplay testing on real
icon sheets, rune atlases, or character art.

## What ships in code today

| Placeholder | What you see | Code |
| --- | --- | --- |
| Team body tint | Attackers red / Defenders blue cubes | `USBPlaceholderArt::TeamColor` + character MID |
| Role rune disc | Colored cylinder above each pawn | `ASBCharacter::RuneDisc` |
| Archetype HUD chips | Letter codes `WAR RNG ASN FLM HEA…` | `USBPlaceholderArt::DrawIconChip` |
| Ability rune bar | 4 dummy glyphs at screen bottom | `ASBSiegeHUD::DrawAbilityRunes` |
| World markers | Floating labels: MAIN GATE, COURTYARD, KEEP RUNE, routes | `ASBWorldMarker` |
| Structure damage colors | Grey → amber → gone | `StructureColor` |

No `.uasset` art required. Engine BasicShapes + dynamic materials + canvas HUD.

## When to add real art

Only after the 20-minute conquest loop feels right. Then swap placeholders for:

### Priority pack (minimum readable game)
1. **10 archetype portraits / icons** (256²) — one per roster entry  
2. **Team banners / spawn pads** — attacker vs defender  
3. **Gate / wall damaged & destroyed meshes or materials** (3 states)  
4. **Courtyard + final-objective props** (capture plinth, keep rune stone)  
5. **4 ability icons per archetype** (or a shared school set: flame / frost / heal / stealth / siege)

### Nice-to-have later
- Unique character meshes / animations  
- Spell VFX with clear readability (§10)  
- Minimap icons + ping glyphs  
- UI chrome for lobby composition panel  

## Drop-in convention (for friends / artists)

Put future assets under:

```
Content/Art/
  Icons/Archetypes/T_Icon_<ArchetypeId>.uasset
  Icons/Abilities/T_Rune_<School>_<Name>.uasset
  Materials/M_Team_Attacker.uasset
  Materials/M_Team_Defender.uasset
  Markers/SM_ObjectiveRune.uasset
```

Then wire soft paths onto `USBCharacterArchetype` (we'll add `Icon` /
`AbilityIcons` soft refs when you're ready). Until those paths are set, the
runtime placeholder kit keeps working.

## Do we need Megascans / marketplace packs now?

**No.** Optional later for fortress dressing. Pilot success is match structure +
hidden comps + siege pacing — not art fidelity.
