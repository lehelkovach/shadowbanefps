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

**Optional for race readability.** Match structure still matters more than art
fidelity — but free skins are worth grabbing when they map cleanly to
Shadowbane races.

## Free race skins (Shadowbane → UE / Fab / Sketchfab)

Pilot roster races today: **Human, Elf, High Elf, Nightshades, Dwarf**.
Classic SB races not yet on the roster (good later silhouettes): **Minotaur,
Centaur, Badawian, Accipitridae, Shedim**.

**Rule:** prefer **CC-BY / Fab free / Epic free** with commercial use. Skip
CC-BY-NC for a shippable game. Always re-check the listing license before
import.

### Claim / import path

1. Fab: open link → Add to library (Epic account) → in Editor **Fab** / **Add
   Feature or Content Pack** → migrate into this project.
2. Sketchfab: Download FBX/GLB → drop under `Content/Art/Characters/<Race>/`
   → Import → retarget to UE5 mannequin if needed (IK Retargeter / Mixamo).
3. Soft-ref later on `USBCharacterArchetype::PawnClass` (or a mesh field we add).

### Shopping list (free / freebie-friendly)

| Shadowbane race | Use for | Free / freebie option | Notes |
| --- | --- | --- | --- |
| **Human** | Ironbrand, Ashwake, Dawnward, Wallwarden | [dReal Warrior Fantasy Character](https://www.fab.com/listings/ec2ce124-b60d-4159-bc1b-989beff7ae4b) (Fab freebie) · UE5 mannequin · [MetaHuman](https://www.unrealengine.com/en-US/metahuman) presets | Best “warrior” silhouette fast |
| **Elf** | Greyfen Warden, Rimebind Wizard | Mixamo stylized characters (Adobe free w/ account) · watch Fab limited-time free elf listings | Full modular elf packs on Fab are usually **paid** |
| **High Elf** | Nightcoil Assassin | Same humanoid base as Human/Elf + darker / hybrid tint | No unique free half-elf mesh needed for pilot |
| **Nightshades** | Underlane Scout | Dark hooded / cursed knight freebies e.g. [Cursed Knight UE5](https://sketchfab.com/3d-models/cursed-knight-ue5-character-game-ready-f204e0ad3af645bcb0ecffd9e9ae9a15) (CC-BY) | Lean stealth / shadow silhouette |
| **Dwarf** | Bastion Templar, Breachwright | Search Fab **Price: Free** + “dwarf”; Sketchfab dwarves often **CC-BY-NC** (skip for commercial) | Scarce truly free commercial dwarves — tinted short humanoid is OK interim |
| **Minotaur** | Future roster / siege bruiser | [Minotaur Berserker](https://sketchfab.com/3d-models/minotaur-berserker-free-game-ready-character-42da47ae59574ed5a6b86b49734294cc) (CC-BY, Mixamo-ready, ~8k tris) | Best free SB-race hit right now |
| **Centaur** | Future roster | Fab centaur listings are mostly **paid** ($50+) | Watch [Fab free biweekly](https://www.unrealengine.com/fabfreecontent) |
| **Badawian** | Future (red / desert dark-elf vibe) | Dark-elf / drow freebies on Sketchfab (verify license) · tinted Elf | Silhouette > lore accuracy for pilot |
| **Accipitridae** | Future (avian) | Rare free; bird-folk usually paid | Defer |
| **Shedim** | Future (giant) | Scaled MetaHuman / tall mannequin | Easy scale hack |

### Fab habit

Every two weeks Epic rotates **limited-time free** on Fab:
https://www.unrealengine.com/fabfreecontent  
Filter **Characters & Creatures** + **Price: Free**. Grab anything minotaur /
elf / dwarf / orc-adjacent while it’s free.

### Drop folder (once you download)

```
Content/Art/Characters/
  Human/
  Elf/
  High Elf/
  Nightshades/
  Dwarf/
  Minotaur/
  Centaur/
```

Keep engine BasicShapes until at least one mesh per current roster race is in
and retargeted — then we wire archetype → skeletal mesh.

## Do we need Megascans for the fortress?

**No.** Optional later for fortress dressing.
