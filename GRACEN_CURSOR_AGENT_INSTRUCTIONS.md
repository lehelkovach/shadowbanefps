# Gracen's Cursor Agent Instructions

**Say to Cursor Agent:**  
`Follow GRACEN_CURSOR_AGENT_INSTRUCTIONS.md and take over client + server development on the dev branch.`

You are **Gracen's local lead agent** for `shadowbanefps`. You own:

1. **Client** — UE 5.5 Editor build, PIE, gameplay iteration, automation tests  
2. **Server package** — Linux dedicated-server cook  
3. **DEV deploy** — hot push to the live OCI VM over **SSH** (not OCI admin)

Cloud Agents write scaffolding/PRs. **You** compile, cook, deploy, and playtest.

---

## Authority / scope

| You own | You do **not** need |
| --- | --- |
| Branch `dev` day-to-day | OCI tenancy admin / Terraform |
| `ShadowbaneFPSEditor` builds | CUDA toolkit |
| `.\scripts\Dev-Push.ps1` cook+deploy | Committing secrets or `Dist/` |
| Connect/playtest `144.24.46.16:7777` | Creating/destroying VMs |
| Fixing compile/cook/deploy errors | |

If SSH deploy key is missing, **stop and ask Lehel** for `%USERPROFILE%\.ssh\shadowbanefps_deploy` (public key must already be on the VM).

---

## Live DEV target

| | |
| --- | --- |
| **Connect** | **`144.24.46.16:7777`** (UDP) |
| **SSH** | `ubuntu@144.24.46.16` |
| **systemd** | `shadowbanefps-server` |
| **Branch** | **`dev`** (hot deploy) → merge to `main` when stable |

Docs: `docs/game-design.md` (source of truth), `docs/SCRIPTS.md`, `docs/TESTING.md`,
`docs/BALANCE_ANALYSIS.md`, `docs/DEV_WORKFLOW.md`, `docs/OCI_DEPLOY.md`, `docs/SETUP.md`,
`docs/reference/SHADOWBANE_LATE_ERA_BALANCE.md`, `BALANCE_AGENT_INSTRUCTIONS.md`

---

## One-time machine setup

Confirm/install:

- Cursor for Windows  
- Epic Launcher → **Unreal Engine 5.5**  
- VS 2022 → **Game development with C++**  
- Git + Git LFS (`git lfs install`) + **Git Bash**  
- NVIDIA Game Ready/Studio drivers  
- UE **Linux cross-compile toolchain** + `LINUX_MULTIARCH_ROOT` (required to cook server)  
- SSH private key: `%USERPROFILE%\.ssh\shadowbanefps_deploy` (or set `SB_SSH_IDENTITY`)

Engine default:

`C:\Program Files\Epic Games\UE_5.5`

Allow Cursor Agent Run Mode to reach the engine tree (Allowlist / Run Everything). Long cooks are normal (30–90+ min).

---

## Bootstrap (first session)

```powershell
git clone https://github.com/lehelkovach/shadowbanefps.git
cd shadowbanefps
git fetch origin

# Prefer `dev`. Fallbacks if it doesn't exist yet:
git checkout dev 2>$null
if ($LASTEXITCODE -ne 0) {
  git checkout main
  git pull origin main
  # If infra/deploy scripts missing on main, use the OCI PR branch once:
  # git fetch origin cursor/oci-shadowbanefps-server-infra-e09a
  # git checkout cursor/oci-shadowbanefps-server-infra-e09a
  git checkout -b dev
  git push -u origin dev
} else {
  git pull origin dev
}
```

Open this folder as the Cursor workspace. Then continue below autonomously.

---

## Standing orders (every session)

### A) Sync + build client
```powershell
git checkout dev
git pull origin dev

.\scripts\Build.ps1 -Target Editor -GenerateProjectFiles
# also useful:  .\scripts\Build.ps1 -Target All
```

Fix compile errors until green. Do not leave the tree broken on `dev`.

### B) Local smoke (PIE)
```powershell
.\scripts\Run-Editor.ps1
# or verbose logs:  .\scripts\Debug-Editor.ps1
# or local dedicated + client:  .\scripts\Debug-Local.ps1
# VS breakpoints:  .\scripts\Open-VS.ps1 -GenerateProjectFiles
```

Expect: Broken Citadel greybox, team colors, HUD chips, world markers.  
Controls: WASD, mouse, LMB fire, `1-0` switch while dead, `R` respawn.

### C) Automation
```powershell
.\scripts\RunAutomationTests.ps1          # all ShadowbaneFPS.*
.\scripts\RunClientTests.ps1              # ShadowbaneFPS.Client.*
.\scripts\RunServerTests.ps1              # ShadowbaneFPS.Server.*
.\scripts\RunIntegrationTests.ps1         # ShadowbaneFPS.Integration.*
```

Paste failures back if red. Logging categories to filter in Output Log / server journal:
`LogShadowbaneServer`, `LogShadowbaneClient`, `LogShadowbaneNet`, `LogShadowbaneTelemetry`.

After playtests, pull `Saved/Telemetry/combat_*.csv` (and match CSV) for build/power balance — damage and kills attributed attacker-build → victim-build.

Balance analyst agent (no UE required):

> Follow `BALANCE_AGENT_INSTRUCTIONS.md` — analyze Saved/Telemetry combat CSVs and recommend nerfs/buffs vs late Ubisoft Shadowbane balance themes.

```powershell
.\scripts\Analyze-CombatBalance.ps1 -Command summary -CompareLateSB
```

### D) Implement / iterate gameplay (your job)
**Source of truth:** `docs/game-design.md` (Draft 1.1+).

**Product identity (non-negotiable):** this is *Shadowbane* FPS. Race / class / promotion / discipline / rune-powers are fundamental — not cosmetic. See design doc **§3** (curated roster table) and keep `SBPilotRoster.cpp` in sync. Do not replace builds with generic hero-shooter roles.

**Pilot economy:** match-local shop like LoL/CS (**§3.1**) — starting gold, ~4 gear slots, buy at staging / on death. No persistent inventory / MMO economy yet (**§16**).

Prioritize the pilot loop:

- Match flow / conquest / overtime already scaffolded  
- Make each roster build *feel* distinct (powers, silhouettes, signatures)  
- First shop slice: gold + 4 slots + tiny catalog  
- Siege devices, intel/pings, lobby / respawn UI, map feel  
- Keep changes on **`dev`**; open PRs to `main` when a slice is stable  

Commit in small, clear commits.

### E) Hot push client+server to DEV (after commits)
```powershell
git add -A
git status
git commit -m "dev: short description of change"
.\scripts\Dev-Push.ps1
```

That script will:

1. Stay on / push **`dev`**  
2. Cook **LinuxServer** (UE dedicated server)  
3. `rsync` to `ubuntu@144.24.46.16`  
4. Restart `shadowbanefps-server`  

Flags when useful:

```powershell
.\scripts\Dev-Push.ps1 -SkipCook      # redeploy last Dist\Server\LinuxServer
.\scripts\Dev-Push.ps1 -SkipDeploy    # cook only
.\scripts\Dev-Push.ps1 -NoPush        # don't git push
```

### F) Connect to live DEV
```powershell
.\scripts\Connect-DevServer.ps1
# → 144.24.46.16:7777
```

If connect fails: check cook succeeded, deploy finished, then:

```powershell
ssh -i $env:USERPROFILE\.ssh\shadowbanefps_deploy ubuntu@144.24.46.16 "sudo systemctl status shadowbanefps-server; sudo journalctl -u shadowbanefps-server -n 80 --no-pager"
```

---

## Definition of done (recurring)

For each feature slice:

- [ ] Builds `ShadowbaneFPSEditor`  
- [ ] PIE sanity OK  
- [ ] Committed + pushed on **`dev`**  
- [ ] `Dev-Push.ps1` deployed (or explicit reason skipped)  
- [ ] Client tested against **`144.24.46.16:7777`** when server code/content changed  
- [ ] Notes/errors reported to Lehel if blocked  

When a slice is stable: open PR **`dev` → `main`** (or tell Lehel to merge).

---

## Quick command card

| Intent | Command |
| --- | --- |
| Take over | Follow this file |
| Sync | `git checkout dev && git pull` |
| Build editor | `Build.bat ShadowbaneFPSEditor Win64 Development ...` |
| Tests | `.\scripts\RunAutomationTests.ps1` |
| Ship to DEV VM | `.\scripts\Dev-Push.ps1` |
| Play on DEV | `.\scripts\Connect-DevServer.ps1` |
| Server logs | `ssh ... journalctl -u shadowbanefps-server -f` |

---

## Blockers → escalate to Lehel

- No SSH deploy key / permission denied to `144.24.46.16`  
- Linux cross-compile toolchain / `LINUX_MULTIARCH_ROOT` missing  
- OCI VM down / UDP 7777 closed  
- Need `main` merge of infra PR (`cursor/oci-shadowbanefps-server-infra-e09a`) before `dev` has scripts  

Do **not** invent OCI API keys or commit PEMs.
