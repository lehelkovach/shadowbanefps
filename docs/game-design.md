# PILOT GAME DESIGN — Shadowbane-Inspired Conquest Siege Test

> A 20-minute multiplayer pilot for hidden team compositions, siege progression, and tactical adaptation.
>
> **Identity:** *Shadowbane FPS* means Shadowbane race / class / promotion / discipline / rune-power builds expressed as pre-built FPS characters. That fantasy is foundational (§3), not optional flavor.
>
> **Pilot economy:** Match-local shop buys (League / CS style) on top of those builds — not a persistent inventory yet. Details will pivot with playtests; long-term path may become MMO / persistent-world economy (§16).

| Match | Format | Teams | Content |
| --- | --- | --- | --- |
| 20 minutes | Asymmetrical siege | 5v5 pilot target | Pre-built characters + match shop |

## Design Premise

Teams assemble coordinated groups from a roster of pre-built characters based on the Shadowbane class system, buy opening gear from a match shop, enter a conquest match without seeing the opposing composition, and adapt as the siege reveals what the enemy brought.

**Pilot focus:** The test is about match structure, hidden compositions, and light buy-phase adaptation. It does **not** yet include leveling, crafting, loot drops, or persistent character progression — those are deferred (§16).

*Draft 1.1 — Testing specification and design rationale*

---

## 1. Executive Summary

This pilot is a self-contained, 20-minute conquest siege match. One team attacks a fortified objective while the other defends it. Both sides organize a group from pre-built Shadowbane-based characters, spend match currency in a simple shop (League-of-Legends / Counter-Strike style), but neither side sees the enemy composition before deployment.

The battle progresses across a staged fortress map: outer approach, breach, courtyard, inner keep, and final objective. Players gather information through scouting and combat, communicate what they discover, and may switch to another available pre-built character (and re-spend leftover / earned gold) after death. The design tests whether hidden compositions, shop choices, and limited mid-match adaptation create a deeper conquest game than a conventional fixed-role siege mode.

**Core loop:** Pick Shadowbane build → buy gear in shop → deploy blind → scout and identify → breach or defend → adapt after losses (swap build / rebuy) → seize or hold the final objective.

### Pilot at a glance

| Element | Pilot specification |
| --- | --- |
| Match length | 20 minutes, plus conditional overtime |
| Team size | 5 attackers versus 5 defenders for the first stable test |
| Character model | Curated roster of complete pre-built characters based on the Shadowbane class system |
| Economy | Match-local gold + shop at staging / on respawn (LoL/CS-like); no persistent stash |
| Team information | Own composition + own buys visible; opposing composition and buys hidden until observed |
| Primary objective | Attackers breach the fortress and complete the final conquest objective; defenders hold until time expires |
| Respawning | Enabled, with staged spawn locations |
| Adaptation | After death: switch pre-built character and/or change shop loadout |
| Map | One asymmetrical fortress with three principal attack routes and staged objectives |

## 2. Design Goals

- Make the pre-match group composition meaningful without allowing it to determine the winner before contact.
- Turn reconnaissance and communication into practical battlefield advantages.
- Create a siege that changes state visibly as structures fall and spawn lines move.
- Allow tactical adaptation without erasing commitment to the team's opening strategy.
- Reward conquest activity—breaching, scouting, holding, repairing, interrupting, and controlling routes—rather than kill count alone.
- Produce a complete testable match using one map and a limited content set.

### Non-goals (pilot)

- Character leveling, trainable skill trees, or out-of-match progression.
- A live persistent world, guild economy, city construction, or long-term territory ownership (**future path — §16**, not this pilot).
- Full inventory management, trading, crafting, or loot drops on corpses.
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
| **Powers / runes** | Readable signature abilities baked into the pre-built (melee pressure, stealth, fire siege burn, heal aura, chill CC, detection, repair). Shop can augment, not replace, the fantasy. |
| **Equipment / resists** | Purchased in the **match shop** (§3.1) as simple gear / rune items. Observable in combat; enemy does not see your buy list. |
| **Role profile** | Coarse lobby read: Damage / Healing / Control / Mobility / Detection / Siege (0–3 each). |

The pilot does **not** ship a full trainer / freeform rune-slotting UI. It **does** ship characters that *read* as finished Shadowbane builds, plus a small shop catalog so opening buys matter.

### 3.1 Match shop (League / CS-style — pilot)

**Intent:** Fast FPS pacing with a short buy moment, not an MMO bank. Think *LoL starting buy* + *CS buy on spawn*, applied to Shadowbane builds.

| Rule | Pilot default (tune in playtests) |
| --- | --- |
| Currency | Match-local **gold** — resets every match; nothing persists |
| Starting gold | Enough for 1–2 meaningful buys (exact numbers TBD in balancing) |
| Income | Small trickle / objective / kill assist bonuses so mid-match rebuys are possible but not infinite |
| When you can buy | **Pre-match staging** and **while dead / at team spawn** before respawn commits (CS-like). No mid-fight shopping. |
| What you buy | A small catalog: weapons/foci, armor/resists, rune charms, utility (detection trinket, repair kit, siege charge). Items modify the selected pre-built's stats / 1–2 power slots. |
| Slots | Keep it dumb for v1: ~**4 gear slots** (e.g. Primary, Secondary/Focus, Armor, Rune/Charm). Not a deep inventory grid. |
| Sell / refund | Allow full refund while still in staging or before leaving spawn after death; no sell mid-fight. |
| Visibility | Own team may see ally buys if useful later; **enemies never see the shop sheet** — only combat signatures (§4). |
| On character swap | Gear either (a) refunds to gold for repurchase, or (b) filters to items legal for the new build — pick one in implementation and keep it consistent; prefer **refund on swap** for the first pilot. |

**Out of scope for shop v1:** crafting, rare drops, shared stash, auctions, account-bound cosmetics economy.

**Assimilation into FPS:** Each bought item must map to something readable in a shooter — fire rate / damage type, resist tint, a pingable trinket VFX, a throwable siege charge — not a spreadsheet buff with no silhouette.

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
- Players then **buy gear** into the 4 slots (§3.1) before first spawn.
- Roster size ~8–12; the table above is the current 10.
- Duplicate limits preserve composition clarity.
- Own team: full race/class/promotion/discipline + role profile + signature + own shop loadout. Enemy team: never shown as a sheet — only observed in the field (§4).
- Expanding the roster means adding Shadowbane-legible builds (new race/class/promotion/discipline + signature), not anonymous FPS roles.

**Design intent:** The team composition is a strategic wager in Shadowbane terms. Shop buys are the second wager (resist the Flame Channeler? stack detection vs Assassin?). Discovery and post-death switches (§9) are how teams adapt without erasing that opening wager.

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

Death creates a temporary tactical opening and an opportunity to reconsider the team's composition **and shop loadout**. On the respawn screen, a player may:

1. Return with the same character, or select another available pre-built character.
2. Open the **match shop** (while dead / at spawn) to refund/rebuy gear with remaining + earned gold.
3. Confirm and respawn after the delay.

This is the pilot's BF/CS-like adaptation beat — whole build swap + gear tweak — without a persistent armory yet.

### Recommended constraints

- Switching and shopping are allowed only after death or at a designated deployment / staging point.
- A short respawn delay applies before re-entry.
- Duplicate or active-character limits preserve composition clarity.
- Heavy siege options may spawn only at appropriate attacker deployment points.
- Defensive emplacement-focused options may require a defender-controlled interior spawn.
- The enemy does not receive an automatic notification of the new selection or buys.

### Why adaptation is limited

The opening group build + opening buys must remain consequential. Switching should let a team answer discovered problems, not instantly rebuild the entire group after every encounter. Respawn time, travel distance, roster limits, gold scarcity, and objective pressure provide the cost of adaptation.

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
- Respawn countdown, available character selections, and **match shop / gold / 4 gear slots**
- Final-objective progress

The interface must **not** reveal unseen enemies, the full opposing roster, enemy respawn selections, hidden loadouts / buys, or exact enemy cooldowns.

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
- Match shop: starting gold, ~4 gear slots, buy at staging / on death
- Small starter catalog (weapon/focus, armor/resist, rune charm, utility)
- One final conquest objective
- 20-minute timer and overtime
- **Basic AI bots + admin spectator client** to populate and watch matches without real players (§12.1)

### 12.1 Bots + admin spectator (populate testing)

Until humans fill queues, the pilot must be playtestable by one developer:

| Mode | Spec |
| --- | --- |
| AI bots | Archetype bots on both teams; each loads a `.sbbot` rule script (`Config/BotScripts/<ArchetypeId>.sbbot`) for chase / fire / hold / retreat. Launch with `?Bots=N`. |
| Admin client | Join as free-cam spectator (`?AdminSpectate=1`); spawn bots via console `AddBots N`; watch siege + HUD; collect `combat_*.csv`. |
| Dev console (planned) | Counter-Strike–style client console: `` ` `` / `~` toggles an on-screen command line for testing (`AddBots`, spectate, cheats, etc.). Today: Unreal `Exec` console. |
| Script | `.\scripts\Run-AdminClient.ps1 -Bots 8` |
| Doc | [`docs/BOTS_AND_ADMIN.md`](./BOTS_AND_ADMIN.md), [`docs/BOT_SCRIPTING.md`](./BOT_SCRIPTING.md), [`docs/DEV_WORKFLOW.md`](./DEV_WORKFLOW.md) |

Bots are **test fixtures**, not ship content. Tweak `.sbbot` files to change behavior without a rebuild; do not block the conquest loop on perfect AI.

### Match telemetry

Combat and shop telemetry exist so we can **nerf/buff builds from evidence**, not vibes.

| Category | Record |
| --- | --- |
| Pacing | Time to first contact, first structural damage, first breach, courtyard capture, and first final-objective attempt |
| Map use | Route selection, player heat maps, repeated choke locations, and abandoned spaces |
| Composition | Character pick rates, opening group patterns, duplicate frequency, and win rate by composition archetype |
| Shop | Opening buy patterns, gold spent by minute, refund/rebuy rate, item pick rates, win rate by item |
| **Combat balance** | Per-hit damage/heal with **attacker build → victim build**, power/item id, amount, HP after, lethal flag; kill attribution same way; end-of-match matchup + power summaries |
| Adaptation | Switch frequency, time of first switch, switches after confirmed intelligence, and post-switch impact |
| Siege | Structure damage (by attacker build + power), repair, device uptime, device destruction, and breach method |
| Outcome | Attacker/defender win rate, average match duration, overtime frequency, and comeback rate |

**Outputs (server / editor):**
- `Saved/Telemetry/match_<stamp>_<session>.csv` — match flow events (+ `BalanceSummary` rows)
- `Saved/Telemetry/combat_<stamp>_<session>.csv` — combat rows for spreadsheet / notebook analysis

**Query / agent:**
- `python3 tools/balance/analyze_combat.py summary Saved/Telemetry --compare-late-sb`
- `.\scripts\Analyze-CombatBalance.ps1` — build/class/power/armor/heal pivots
- Agent prompt: [`BALANCE_AGENT_INSTRUCTIONS.md`](../BALANCE_AGENT_INSTRUCTIONS.md)
- Late Ubisoft baseline (P22–P23 → 2009 shutdown): [`docs/reference/SHADOWBANE_LATE_ERA_BALANCE.md`](./reference/SHADOWBANE_LATE_ERA_BALANCE.md)

Use combat CSV to answer: which builds delete which builds, which powers over-perform, lethality spikes after certain shop buys.

### Playtest questions

- Does hiding the enemy composition create strategy or merely confusion?
- Can players infer enough about the opposing group through normal play?
- Do teams coordinate coherent opening compositions **and** opening buys?
- Does the shop feel like LoL/CS (fast, consequential) or like busywork?
- Does the opening group build matter after character switching becomes available?
- Are switches deliberate responses to information, or constant opportunistic counter-picks?
- Are all three attack routes used for meaningful reasons?
- Do siege devices require teamwork and create counterplay?
- Does the battle advance through distinct spaces, or remain stuck at one choke point?
- Can defenders recover after losing the outer wall?
- Can attackers recover after a failed breach?
- Does the final assault feel climactic and understandable?
- Is 20 minutes the correct duration for a complete conquest arc?
- Does match-local gold teach anything useful before a future persistent economy?
- Which build→build matchups or powers look overtuned from combat CSV?

## 13. Principal Design Risks

- **Opening composition becomes irrelevant:** Too much switching turns the match into continuous counter-picking. Use meaningful respawn, travel, availability, and objective costs.
- **Opening composition decides the match:** Too little adaptation makes a blind unfavorable matchup feel predetermined. Ensure several routes and limited substitutions can change the tactical problem.
- **Hidden information feels arbitrary:** Unclear silhouettes and effects make reconnaissance unreliable. Prioritize readable presentation before adding more builds.
- **The siege becomes one choke-point brawl:** If alternate routes lack value, the map fails. Each route must create a different strategic opportunity and defensive burden.
- **Attackers or defenders snowball:** Use staged spawns, fallback terrain, vulnerable forward positions, and limited resupply to preserve comeback possibilities.
- **Scope expands into the full MMO concept too early:** Keep the pilot limited to one map, one match type, a controlled roster, match-local shop, and the new mechanics being tested. Persist world systems only after the 20-minute loop works (§16).
- **Shop becomes the whole game:** If buys dominate over builds and siege skill, shrink the catalog and buff base archetypes.

## 14. Pilot Success Criteria

- Most teams form recognizable opening group strategies rather than selecting independently.
- Opening shop buys feel intentional (not random) and readable in combat.
- Players can identify important enemy capabilities through observation and communication.
- At least two attack routes remain viable across repeated matches.
- The siege typically progresses through more than one map stage.
- Character switching / rebuying occurs often enough to demonstrate adaptation but not so often that the opening composition is meaningless.
- Support, scouting, siege, repair, and objective play materially influence victory.
- Both attackers and defenders can produce credible comebacks.
- Matches usually conclude near the intended duration and produce a clear final contest.
- Players describe losses in terms of identifiable tactical decisions rather than opaque build surprises.

## 15. Design Summary

The pilot compresses the group-building and conquest appeal of Shadowbane into a single short-form siege match, with a League/CS-style match shop as the first economy layer. Its original contribution is the interaction among blind team composition, shop adaptation, battlefield intelligence, staged fortress conquest, and constrained character substitution after death.

The design succeeds when the opening group build and buys matter, discovery changes decisions, siege objectives move the front line, and adaptation creates reversals without reducing the match to endless counter-swapping.

## 16. Future path (not this pilot)

Playtests may later pivot this toward a **real MMO or persistent-world multiplayer** game. When that happens, expect:

- Account / character persistence, training, and broader Shadowbane build crafting
- Persistent inventory, crafting, and economy (shops become vendors / auction / guild banks)
- Territory, cities, and longer conquest seasons

Until then: **match-local gold + small shop + curated pre-builts**. Do not build persistence systems in the pilot codebase unless the design is explicitly updated again.
