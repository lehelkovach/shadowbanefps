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

Docs: `docs/DEV_WORKFLOW.md`, `docs/OCI_DEPLOY.md`, `docs/SETUP.md`, `docs/TESTING.md`, `docs/game-design.md`

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

$UE = "C:\Program Files\Epic Games\UE_5.5"
$PROJ = "$PWD\ShadowbaneFPS.uproject"

& "$UE\Engine\Build\BatchFiles\Build.bat" -projectfiles -project="$PROJ" -game -engine -progress
& "$UE\Engine\Build\BatchFiles\Build.bat" ShadowbaneFPSEditor Win64 Development -Project="$PROJ" -WaitMutex
```

Fix compile errors until green. Do not leave the tree broken on `dev`.

### B) Local smoke (PIE)
```powershell
& "$UE\Engine\Binaries\Win64\UnrealEditor.exe" "$PROJ"
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

### D) Implement / iterate gameplay (your job)
Work from `docs/game-design.md`. Prioritize the pilot loop:

- Match flow / conquest / overtime already scaffolded  
- Combat readability, archetypes, siege devices, intel/pings, lobby UI, map feel  
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
