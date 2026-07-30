#!/usr/bin/env python3
"""
Query ShadowbaneFPS combat_*.csv telemetry for balance analysis.

Examples:
  python3 tools/balance/analyze_combat.py build-vs-build Saved/Telemetry
  python3 tools/balance/analyze_combat.py class-vs-class Saved/Telemetry
  python3 tools/balance/analyze_combat.py power-vs-victim-class Saved/Telemetry
  python3 tools/balance/analyze_combat.py power-vs-armor Saved/Telemetry
  python3 tools/balance/analyze_combat.py heals Saved/Telemetry
  python3 tools/balance/analyze_combat.py summary Saved/Telemetry --compare-late-sb
  python3 tools/balance/analyze_combat.py query Saved/Telemetry --atk-class Channeler --vic-class Templar
"""

from __future__ import annotations

import argparse
import csv
import json
import sys
from collections import defaultdict
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Tuple


ROOT = Path(__file__).resolve().parents[2]
ROSTER_PATH = Path(__file__).resolve().parent / "roster.json"


def load_roster() -> Dict[str, dict]:
    data = json.loads(ROSTER_PATH.read_text(encoding="utf-8"))
    return data.get("archetypes", {})


def find_combat_csvs(path: Path) -> List[Path]:
    if path.is_file() and path.name.startswith("combat_") and path.suffix == ".csv":
        return [path]
    if not path.exists():
        return []
    return sorted(path.glob("combat_*.csv"))


def enrich(row: dict, roster: Dict[str, dict]) -> dict:
    atk = roster.get(row.get("attacker_arch", ""), {})
    vic = roster.get(row.get("victim_arch", ""), {})
    out = dict(row)
    out["attacker_class"] = atk.get("class", "")
    out["attacker_race"] = atk.get("race", "")
    out["attacker_discipline"] = atk.get("discipline", "")
    out["victim_class"] = vic.get("class", "")
    out["victim_race"] = vic.get("race", "")
    out["victim_discipline"] = vic.get("discipline", "")
    try:
        out["amount_f"] = float(row.get("amount") or 0)
    except ValueError:
        out["amount_f"] = 0.0
    out["lethal_i"] = 1 if str(row.get("lethal", "0")).strip() in ("1", "true", "True") else 0
    return out


def load_rows(paths: Iterable[Path], roster: Dict[str, dict]) -> List[dict]:
    rows: List[dict] = []
    for p in paths:
        with p.open(newline="", encoding="utf-8") as f:
            reader = csv.DictReader(f)
            for row in reader:
                rows.append(enrich(row, roster))
    return rows


def pivot(
    rows: List[dict],
    left: str,
    right: str,
    kind: Optional[str] = None,
) -> List[Tuple[str, str, float, int, int]]:
    damage = defaultdict(float)
    hits = defaultdict(int)
    kills = defaultdict(int)
    for r in rows:
        if kind and r.get("kind") != kind:
            if kind == "Damage" and r.get("kind") == "Kill":
                # kills counted separately
                pass
            elif kind == "Damage" and r.get("kind") != "Damage":
                continue
            elif kind != "Damage":
                continue
        key = (r.get(left) or "?", r.get(right) or "?")
        if r.get("kind") == "Damage" and (kind in (None, "Damage")):
            damage[key] += r["amount_f"]
            hits[key] += 1
        if r.get("kind") == "Heal" and kind == "Heal":
            damage[key] += r["amount_f"]
            hits[key] += 1
        if r.get("kind") == "Kill" and (kind in (None, "Kill", "Damage")):
            kills[key] += 1
    keys = set(damage) | set(kills) | set(hits)
    out = []
    for k in keys:
        out.append((k[0], k[1], damage[k], hits[k], kills[k]))
    out.sort(key=lambda t: (-t[2], -t[4], t[0], t[1]))
    return out


def print_table(title: str, rows: List[Tuple[str, str, float, int, int]], left: str, right: str) -> None:
    print(f"\n=== {title} ===")
    print(f"{left:28} {right:28} {'damage':>10} {'hits':>6} {'kills':>6}")
    print("-" * 84)
    if not rows:
        print("(no rows)")
        return
    for a, b, dmg, hits, kills in rows[:40]:
        print(f"{a:28} {b:28} {dmg:10.1f} {hits:6d} {kills:6d}")
    if len(rows) > 40:
        print(f"... {len(rows) - 40} more matchups")


LATE_SB_NOTES = """
Late official Shadowbane balance baseline (Ubisoft era → shutdown 2009-05-01)
----------------------------------------------------------------------------
Primary sources (Morloch Wiki / community archive of official notes):
  - Patch 22 "Shadowbane Reboot" (2008-03-25): full reset; armor redistribute;
    bow/crossbow power damage % down; snares nerfed; caster stealth capped;
    melee range up slightly; class armor sets; siege engine player AoE removed;
    wall archers removed. Intent: make the game rebalanceable after unfixable items.
  - Patch 23 (2008-08-01): last major published live balance pass before closure.
    Weapon skill ladder (25/50/75/100/110%); class-specific staves/wands with baked
    skills; glass weapons → bleeding/poison; mage HP/level up; Channeler/Fury/
    Necro/Wizard HP/level up; snares retuned 11–60%; power-block immunity not
    dispelled; armor/weapon enchantment cleanups.

Pilot comparison heuristics (not 1:1 ports):
  - Fire / bow siege pressure should not delete tanks without commitment (P22 bow nerfs).
  - Stealth / assassin burst should be strong but answerable by detection (P21/P22 stealth caps).
  - Snares / frost control: impactful but not perma-root (P22/P23 snare bands).
  - Heal auras: sustain pushes, not immortality (pilot success criteria).
  - Structure/siege damage is a first-class win condition, not just player DPS.
  - Prefer evidence from combat_*.csv over nostalgia; cite late-SB themes when recommending nerfs/buffs.
"""


def cmd_summary(rows: List[dict], compare: bool) -> None:
    print(f"Loaded {len(rows)} combat rows")
    by_kind = defaultdict(int)
    for r in rows:
        by_kind[r.get("kind", "?")] += 1
    for k, v in sorted(by_kind.items()):
        print(f"  {k}: {v}")
    print_table("Build vs Build (damage/kills)", pivot(rows, "attacker_arch", "victim_arch"), "attacker_arch", "victim_arch")
    print_table("Class vs Class", pivot(rows, "attacker_class", "victim_class"), "attacker_class", "victim_class")
    print_table("Power vs Victim Class", pivot(rows, "power", "victim_class", kind="Damage"), "power", "victim_class")
    heals = pivot(rows, "attacker_arch", "victim_arch", kind="Heal")
    print_table("Heal / buff attribution (healer → target)", heals, "healer_arch", "target_arch")
    if compare:
        print(LATE_SB_NOTES)


def cmd_query(rows: List[dict], args: argparse.Namespace) -> None:
    filtered = []
    for r in rows:
        if args.kind and r.get("kind") != args.kind:
            continue
        if args.atk_arch and r.get("attacker_arch") != args.atk_arch:
            continue
        if args.vic_arch and r.get("victim_arch") != args.vic_arch:
            continue
        if args.atk_class and r.get("attacker_class") != args.atk_class:
            continue
        if args.vic_class and r.get("victim_class") != args.vic_class:
            continue
        if args.power and r.get("power") != args.power:
            continue
        if args.power_contains and args.power_contains.lower() not in (r.get("power") or "").lower():
            continue
        filtered.append(r)
    print(f"Matched {len(filtered)} rows")
    total = sum(r["amount_f"] for r in filtered if r.get("kind") == "Damage")
    kills = sum(1 for r in filtered if r.get("kind") == "Kill")
    heals = sum(r["amount_f"] for r in filtered if r.get("kind") == "Heal")
    print(f"Damage={total:.1f}  Kills={kills}  Heal={heals:.1f}")
    print_table("Filtered build vs build", pivot(filtered, "attacker_arch", "victim_arch"), "attacker_arch", "victim_arch")


def main() -> int:
    parser = argparse.ArgumentParser(description="Analyze ShadowbaneFPS combat telemetry CSVs")
    parser.add_argument(
        "command",
        choices=[
            "summary",
            "build-vs-build",
            "class-vs-class",
            "power-vs-victim-arch",
            "power-vs-victim-class",
            "power-vs-armor",
            "heals",
            "query",
        ],
    )
    parser.add_argument("path", nargs="?", default="Saved/Telemetry", help="combat_*.csv file or directory")
    parser.add_argument("--compare-late-sb", action="store_true", help="Print late Ubisoft-era balance heuristics")
    parser.add_argument("--atk-arch")
    parser.add_argument("--vic-arch")
    parser.add_argument("--atk-class")
    parser.add_argument("--vic-class")
    parser.add_argument("--power")
    parser.add_argument("--power-contains")
    parser.add_argument("--kind", choices=["Damage", "Heal", "Kill"])
    args = parser.parse_args()

    roster = load_roster()
    paths = find_combat_csvs(Path(args.path))
    if not paths:
        print(f"No combat_*.csv under {args.path}", file=sys.stderr)
        print("Run a match first, or pass an explicit CSV path.", file=sys.stderr)
        return 2

    rows = load_rows(paths, roster)
    if not rows:
        print("CSV(s) found but empty.")
        return 1

    if args.command == "summary":
        cmd_summary(rows, args.compare_late_sb)
    elif args.command == "build-vs-build":
        print_table("Build vs Build", pivot(rows, "attacker_arch", "victim_arch"), "attacker_arch", "victim_arch")
    elif args.command == "class-vs-class":
        print_table("Class vs Class", pivot(rows, "attacker_class", "victim_class"), "attacker_class", "victim_class")
    elif args.command == "power-vs-victim-arch":
        print_table("Power vs Victim Build", pivot(rows, "power", "victim_arch", kind="Damage"), "power", "victim_arch")
    elif args.command == "power-vs-victim-class":
        print_table("Power vs Victim Class", pivot(rows, "power", "victim_class", kind="Damage"), "power", "victim_class")
    elif args.command == "power-vs-armor":
        # Armor/resist shop items and tank disciplines as stand-ins until item ids mature.
        armorish = [
            r
            for r in rows
            if r.get("kind") == "Damage"
            and (
                "Armor" in (r.get("power") or "")
                or "Resist" in (r.get("power") or "")
                or r.get("victim_discipline") in ("Bulwark", "Fortress", "Blessed Mantle")
                or "Armor" in (r.get("extra") or "")
            )
        ]
        print_table(
            "Power vs armored / resist-ish victims",
            pivot(armorish, "power", "victim_arch"),
            "power",
            "victim_arch",
        )
    elif args.command == "heals":
        print_table("Heals / buffs", pivot(rows, "attacker_arch", "victim_arch", kind="Heal"), "source_arch", "target_arch")
    elif args.command == "query":
        cmd_query(rows, args)

    if args.compare_late_sb and args.command != "summary":
        print(LATE_SB_NOTES)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
