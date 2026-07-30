# PILOT GAME DESIGN — Shadowbane-Inspired Conquest Siege Test

> A 20-minute multiplayer pilot for hidden team compositions, siege progression, and tactical adaptation.
>
> **Identity:** *Shadowbane FPS* means Shadowbane race / class / promotion / discipline / rune-power builds expressed as pre-built FPS characters. That fantasy is foundational (§3), not optional flavor.

| Match | Format | Teams | Content |
| --- | --- | --- | --- |
| 20 minutes | Asymmetrical siege | 5v5 pilot target | Pre-built characters |

## Design Premise

Teams assemble coordinated groups from a roster of pre-built characters based on the Shadowbane class system, enter a conquest match without seeing the opposing composition, and adapt as the siege reveals what the enemy brought.

**Pilot focus:** The test is about the match structure and the information game. It does **not** include leveling, training, loot acquisition, crafting, or persistent character progression.

*Draft 1.0 — Testing specification and design rationale*

---

## 1. Executive Summary

This pilot is a self-contained, 20-minute conquest siege match. One team attacks a fortified objective while the other defends it. Both sides organize a group from pre-built Shadowbane-based characters, but neither side sees the enemy composition before deployment.

The battle progresses across a staged fortress map: outer approach, breach, courtyard, inner keep, and final objective. Players gather information through scouting and combat, communicate what they discover, and may switch to another available pre-built character after death. The design tests whether hidden compositions and limited mid-match adaptation create a deeper conquest game than a conventional fixed-role siege mode.

**Core loop:** Choose a group build -> deploy blind -> scout and identify -> breach or defend -> adapt after losses -> seize or hold the final objective.

### Pilot at a glance

| Element | Pilot specification |
| --- | --- |
| Match length | 20 minutes, plus conditional overtime |
| Team size | 5 attackers versus 5 defenders for the first stable test |
| Character model | Curated roster of complete pre-built characters based on the Shadowbane class system |
| Team information | Own composition visible; opposing composition hidden until observed |
| Primary objective | Attackers breach the fortress and complete the final conquest objective; defenders hold until time expires |
| Respawning | Enabled, with staged spawn locations |
| Adaptation | Players may change to another available pre-built character after death |
| Map | One asymmetrical fortress with three principal attack routes and staged objectives |

## 2. Design Goals

- Make the pre-match group composition meaningful without allowing it to determine the winner before contact.
- Turn reconnaissance and communication into practical battlefield advantages.
- Create a siege that changes state visibly as structures fall and spawn lines move.
- Allow tactical adaptation without erasing commitment to the team's opening strategy.
- Reward conquest activity—breaching, scouting, holding, repairing, interrupting, and controlling routes—rather than kill count alone.
- Produce a complete testable match using one map and a limited content set.

### Non-goals

- Character leveling, trainable skills, or in-match progression.
- A persistent world, guild economy, city construction, or long-term territory ownership.
- A complete reproduction of every Shadowbane class and build combination.
- Fully dynamic terrain destruction or large-army simulation.
- A hero-shooter roster designed around generic modern roles.

## 3. Character and Team Selection

**This is foundational to the product identity.** The project is named *Shadowbane FPS* because the playable fantasy is Shadowbane's race / class / promotion / discipline / rune-power language, compressed into pre-built FPS characters — not a generic hero shooter with Shadowbane skin. Match structure (§5–§9) is the *test*; Shadowbane builds are the *content*.

Agents implementing gameplay must treat the following as first-class requirements, not flavor text.

### Shadowbane build language (required vocabulary)

Every pre-built character is authored with this stack (shown to the **owning team** in lobby / respawn UI):

| Layer | Meaning in the pilot |
| --- | --- |
| **Race** | Fantasy lineage that players recognize (Human, Elf, Aelfborn, Dwarf, Shade, …). Affects silhouette and expected fantasy; may later bias resists / mobility. |
| **Class** | Base vocation (Warrior, Ranger, Assassin, Channeler, Healer, Wizard, Thief, Templar, …). |
| **Promotion** | Advanced path (Warlord, Huntress, Nightstalker, Furia, Prelate, Warlock, …). |
| **Discipline** | Training / school (Blade Weaving, Way of the Bow, Shadowmantle, Flame, Frost, Siegecraft, …). |
| **Powers / runes** | Readable signature abilities (melee pressure, stealth, fire siege burn, heal aura, chill CC, detection, repair). Placeholder HUD “runes” stand in until real ability VFX/SFX land. |
| **Equipment / resists** | Implied by the pre-built; not a loot/crafting loop. Observable in combat, not as a full sheet to enemies. |
| **Role profile** | Coarse lobby read: Damage / Healing / Control / Mobility / Detection / Siege (0–3 each). |

The pilot does **not** ship a full trainer / rune-slotting UI. It **does** ship characters that *read* as finished Shadowbane builds, with distinct audiovisual signatures so hidden-composition play works (§4).

### Curated pilot roster (authoritative list)

Canonical runtime source: `Source/ShadowbaneFPS/Characters/SBPilotRoster.cpp`. Keep this table and that file in sync.

| Id | Display name | Race | Class | Promotion | Discipline | Signature | Side | Dup |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `Warrior_Blade` | Ironbrand Warrior | Human | Warrior | Warlord | Blade Weaving | heavy melee pressure | Both | 2 |
| `Ranger_Scout` | Greyfen Ranger | Elf | Ranger | Huntress | Way of the Bow | long-range bow / high mobility | Both | 2 |
| `Assassin_Shadow` | Nightcoil Assassin | Aelfborn | Assassin | Nightstalker | Shadowmantle | stealth / close burst | Both | 1 |
| `Channeler_Flame` | Ashwake Channeler | Human | Channeler | Furia | Flame | fire magic / siege burn | Both | 1 |
| `Healer_Prelate` | Dawnward Prelate | Human | Healer | Prelate | Blessed Mantle | sustained healing aura | Both | 2 |
| `Wizard_Frost` | Rimebind Wizard | Elf | Wizard | Warlock | Frost | crowd control / chill zones | Both | 1 |
| `Scout_Thief` | Underlane Scout | Shade | Thief | Saboteur | Silent Step | scouting / detection / sabotage | Both | 2 |
| `Templar_Bulwark` | Bastion Templar | Dwarf | Templar | Paladin | Bulwark | frontline hold / light heal | Both | 2 |
| `Siege_Engineer` | Breachwright Engineer | Dwarf | Warrior | Huntmaster | Siegecraft | siege device / structure damage | Attackers | 1 |
| `Defender_Warden` | Wallwarden | Human | Warrior | Warlord | Fortress | repair / emplacement defense | Defenders | 2 |

Coverage the roster must keep: melee, ranged, stealth, fire siege mage, healer, frost CC, detection/scout, tank/support, attacker siege specialist, defender repair/emplacement.

### Selection rules for the pilot

- Every option is fully pre-built and immediately playable (a **character**, not a loose skill bundle).
- Roster size ~8–12; the table above is the current 10.
- Duplicate limits preserve composition clarity.
- Own team: full race/class/promotion/discipline + role profile + signature. Enemy team: never shown as a sheet — only observed in the field (§4).
- Expanding the roster means adding Shadowbane-legible builds (new race/class/promotion/discipline + signature), not anonymous FPS roles.

**Design intent:** The team composition is a strategic wager in Shadowbane terms. A Flame Channeler + Siege Engineer push answers differently than a stealth/scout line; discovery and post-death switches (§9) are how teams adapt without erasing that opening wager.

## 4. Hidden Composition and Battlefield Intelligence

The enemy lineup is hidden at match start. Players learn what they are facing by seeing silhouettes, weapons, spell effects, movement, defensive reactions, siege behavior, and other observable evidence. The game should reveal enough for informed adaptation without automatically exposing a complete build sheet.

### Information tools

- Contextual pings for enemy sightings, breach points, siege devices, traps, and regroup locations.
- Short-lived map markers for recently observed enemies rather than permanent tracking.
- Clear silhouettes and audiovisual signatures for major powers and battlefield roles.
- A concise death recap that explains what killed the player without revealing the enemy's full hidden configuration.
- Optional confirmed-intelligence labels such as "stealth observed," "high fire resistance suspected," or "repair activity detected."

### Deception

Teams may conceal key characters, delay signature powers, stage a false frontal push, or switch plans after the enemy commits its counters. The system should permit deception, but visual readability must remain strong enough that uncertainty comes from incomplete information rather than confusing effects.

## 5. Match Structure and Victory

- **Attacker objective:** Breach the fortress, advance the conquest state, and complete the final objective before the 20-minute timer expires.
- **Defender objective:** Delay or repel the assault and keep the final objective secure until the timer expires.
- **Final objective:** The recommended pilot objective is an interruptible capture or destruction interaction inside the inner keep. Attackers must maintain control long enough to complete it; defenders can interrupt by contesting, displacing, or eliminating the interacting attackers. Progress may persist in segments or decay slowly, depending on testing.
- **Overtime:** If time expires during an active final-objective contest, overtime continues until attackers complete the objective or defenders clear the area and prevent renewed progress for a short confirmation period.
- **Kills:** Kills create temporary numerical and positional advantages but do not directly score victory. A team wins by changing and controlling the state of the battlefield.

## 6. Pilot Map: The Broken Citadel

The Broken Citadel is a compact asymmetrical fortress map designed to move a full match through several different combat spaces. The fortress is already damaged enough to support multiple plausible breaches, but the defenders begin with control of the walls, interior routes, and final objective.

High-level topology:

- North approach — Wall / tower — Upper keep
- Attacker staging — Siege field — Main gate — Courtyard — Inner keep / final objective
- South approach — Service entrance — Lower keep — Defender deployment

Primary flow: attacker staging -> siege field -> breach -> courtyard -> inner keep

### Major zones

- **Attacker staging ground:** Protected initial deployment, character selection, route planning, and access to initial siege equipment. First contact should occur within roughly 30–45 seconds.
- **Outer siege field:** Open and broken terrain where attackers establish pressure and defenders attempt to disrupt siege preparation.
- **Main gate route:** The shortest, most visible, and most heavily defended path. It supports direct group pushes and siege equipment.
- **Northern wall route:** An exposed vertical or elevated approach that rewards mobility, ranged control, and seizure of wall positions.
- **Southern service route:** A slower concealed route through tunnels, ruins, or service passages that rewards scouting, stealth, detection, and close-range fighting.
- **Courtyard:** The transition from outer siege to interior conquest. Capturing it unlocks an attacker forward spawn and forces defender fallback.
- **Inner keep:** Tighter combat spaces, defender shortcuts, and the final objective. The map should culminate here rather than end at the first breach.

## 7. Siege and Conquest Mechanics

### Breach options

- Destroy the main gate with direct damage or a siege device.
- Destroy or open a weaker secondary wall section.
- Reach a side mechanism from the wall or service route and open access from inside.
- Sabotage defensive infrastructure to make another route viable.

### Siege devices

The minimum pilot should include one attacker-operated siege device and one defender-operated emplacement. Devices must be powerful enough to shape the fight but vulnerable enough to require protection, positioning, and counterplay.

- **Attacker device:** a battering ram, cannon, ballista, or magical equivalent used primarily against structures.
- **Defender emplacement:** a fixed weapon or defensive mechanism controlling a clear lane or breach zone.
- Operation exposes the user, limits mobility, and creates an obvious tactical target.
- Destroyed devices remain unavailable long enough for the destruction to matter.

### Destructible structures

Destruction is predetermined and functional rather than fully dynamic. Each structure has intact, damaged, and destroyed states with a clear gameplay consequence.

- Main gate or portcullis
- Secondary breach wall
- Defensive emplacement
- Detection or warning beacon
- Barricade or reinforcement mechanism
- Forward spawn anchor, if testing counterattacks against reinforcement lines

### Repair

Defenders may repair damaged structures through character abilities or map interactions. Repairing requires exposure and can be interrupted. A completely destroyed structure should remain destroyed for the rest of the match in the first pilot.

### Conquest state

The map changes ownership in stages. Capturing the courtyard moves the attacker spawn forward and pushes the defender spawn inward. This creates a visible front line, reduces repetitive travel, and prevents defenders from endlessly spawning behind territory they have lost.

## 8. Match Phases and Pacing

| Phase | Target window | Purpose |
| --- | --- | --- |
| Phase 1 — Reconnaissance and outer siege | Approx. minutes 0–6 | Teams identify compositions, contest the approaches, establish siege pressure, and test the enemy's route coverage. |
| Phase 2 — Breach and courtyard conquest | Approx. minutes 5–14 | Attackers force one or more entries, defenders choose where to fall back, and control of the courtyard determines reinforcement distance. |
| Phase 3 — Inner assault | Approx. minutes 12–20 | The fight compresses into the inner fortress, where attackers must create enough control to complete the final objective. |

The time windows are pacing targets, not hard locks. A strong attack may breach early; a strong defense may hold the outer wall for most of the match. The map should still provide anti-stalemate tools so the entire 20 minutes cannot collapse into one gate choke by default.

## 9. Respawning and Tactical Adaptation

Death creates a temporary tactical opening and an opportunity to reconsider the team's composition. On the respawn screen, a player may return with the same character or select another available pre-built character.

### Recommended constraints

- Switching is allowed only after death or at a designated deployment point.
- A short respawn delay applies before re-entry.
- Duplicate or active-character limits preserve composition clarity.
- Heavy siege options may spawn only at appropriate attacker deployment points.
- Defensive emplacement-focused options may require a defender-controlled interior spawn.
- The enemy does not receive an automatic notification of the new selection.

### Why adaptation is limited

The opening group build must remain consequential. Switching should let a team answer discovered problems, not instantly rebuild the entire group after every encounter. Respawn time, travel distance, roster limits, and objective pressure provide the cost of adaptation.

## 10. Combat and Interaction Requirements

The pilot should preserve the recognizable capabilities and group interactions of its Shadowbane-derived builds while presenting them through a readable, responsive multiplayer combat model.

- **Readability:** Major attacks, defensive powers, healing, stealth, detection, siege damage, and crowd control require distinct visual and audio feedback.
- **Positioning:** Line of sight, elevation, cover, choke points, and route control should matter as much as raw damage output.
- **Crowd control:** Hard control must remain brief, telegraphed, and resistant to indefinite chaining.
- **Stealth and detection:** Stealth creates local uncertainty; attacks, objectives, proximity, or detection can reveal the user. Detection should protect areas rather than expose the entire map.
- **Healing and support:** Support should sustain coordinated pushes without making concentrated targets functionally immortal.
- **Structure interaction:** Characters and siege devices need clear rules for damaging, repairing, disabling, and contesting structures.

## 11. User Interface and Communication

- Match timer and overtime state
- Current conquest stage and active objective
- Structural health and breach status
- Available spawn locations
- Teammate positions and team pings
- Recently observed enemy markers
- Respawn countdown and available character selections
- Final-objective progress

The interface must **not** reveal unseen enemies, the full opposing roster, enemy respawn selections, hidden loadouts, or exact enemy cooldowns.

## 12. Testing Plan

### Minimum viable content

- One complete fortress map
- 5v5 multiplayer
- A curated roster of approximately 8–12 pre-built characters
- Three attack routes
- One main gate and one alternate breach
- One attacker siege device and one defender emplacement
- Staged attacker and defender spawns
- Hidden enemy composition
- Basic pings and temporary enemy markers
- Character switching after death
- One final conquest objective
- 20-minute timer and overtime

### Match telemetry

| Category | Record |
| --- | --- |
| Pacing | Time to first contact, first structural damage, first breach, courtyard capture, and first final-objective attempt |
| Map use | Route selection, player heat maps, repeated choke locations, and abandoned spaces |
| Composition | Character pick rates, opening group patterns, duplicate frequency, and win rate by composition archetype |
| Adaptation | Switch frequency, time of first switch, switches after confirmed intelligence, and post-switch impact |
| Siege | Structure damage, repair, device uptime, device destruction, and breach method |
| Outcome | Attacker/defender win rate, average match duration, overtime frequency, and comeback rate |

### Playtest questions

- Does hiding the enemy composition create strategy or merely confusion?
- Can players infer enough about the opposing group through normal play?
- Do teams coordinate coherent opening compositions?
- Does the opening group build matter after character switching becomes available?
- Are switches deliberate responses to information, or constant opportunistic counter-picks?
- Are all three attack routes used for meaningful reasons?
- Do siege devices require teamwork and create counterplay?
- Does the battle advance through distinct spaces, or remain stuck at one choke point?
- Can defenders recover after losing the outer wall?
- Can attackers recover after a failed breach?
- Does the final assault feel climactic and understandable?
- Is 20 minutes the correct duration for a complete conquest arc?

## 13. Principal Design Risks

- **Opening composition becomes irrelevant:** Too much switching turns the match into continuous counter-picking. Use meaningful respawn, travel, availability, and objective costs.
- **Opening composition decides the match:** Too little adaptation makes a blind unfavorable matchup feel predetermined. Ensure several routes and limited substitutions can change the tactical problem.
- **Hidden information feels arbitrary:** Unclear silhouettes and effects make reconnaissance unreliable. Prioritize readable presentation before adding more builds.
- **The siege becomes one choke-point brawl:** If alternate routes lack value, the map fails. Each route must create a different strategic opportunity and defensive burden.
- **Attackers or defenders snowball:** Use staged spawns, fallback terrain, vulnerable forward positions, and limited resupply to preserve comeback possibilities.
- **Scope expands into the full MMO concept:** Keep the pilot limited to one map, one match type, a controlled roster, and the new mechanics being tested.

## 14. Pilot Success Criteria

- Most teams form recognizable opening group strategies rather than selecting independently.
- Players can identify important enemy capabilities through observation and communication.
- At least two attack routes remain viable across repeated matches.
- The siege typically progresses through more than one map stage.
- Character switching occurs often enough to demonstrate adaptation but not so often that the opening composition is meaningless.
- Support, scouting, siege, repair, and objective play materially influence victory.
- Both attackers and defenders can produce credible comebacks.
- Matches usually conclude near the intended duration and produce a clear final contest.
- Players describe losses in terms of identifiable tactical decisions rather than opaque build surprises.

## 15. Design Summary

The pilot compresses the group-building and conquest appeal of Shadowbane into a single short-form siege match. Its original contribution is the interaction among blind team composition, battlefield intelligence, staged fortress conquest, and constrained character substitution after death.

The design succeeds when the opening group build matters, discovery changes decisions, siege objectives move the front line, and adaptation creates reversals without reducing the match to endless counter-swapping. Everything outside that test—progression, economy, guild politics, and persistent territory—should remain out of scope until this 20-minute conquest loop proves itself.
