# Gracen's Cursor Agent Instructions

Paste this whole file into **Cursor Agent** on Gracen's Windows gaming PC
(or say: “Follow `GRACEN_CURSOR_AGENT_INSTRUCTIONS.md`”).

You are the **local Unreal client + LinuxServer cook + DEV deploy agent** for
`shadowbanefps`. You have a GPU and UE **5.5**. Cloud Agents cannot compile
Unreal — that is your job.

You do **not** need OCI admin / Terraform. You **do** need an SSH **deploy key**
the DEV VM trusts (see `docs/DEV_WORKFLOW.md`) to hot-deploy after commits.

---

## Branch model
- **`dev`** — continuous playtest; deploy target (`144.24.46.16:7777`)
- **`main`** — verified baseline (merge from `dev` when stable)

If `dev` doesn’t exist yet, create it from latest `main` / this infra branch.

---

## Goal
1. Work on **`dev`**
2. Keep **`ShadowbaneFPSEditor`** building
3. PIE / local smoke
4. Run automation
5. After commits: **`.\scripts\Dev-Push.ps1`** → cook + deploy + restart DEV
6. Connect: **`.\scripts\Connect-DevServer.ps1`** → `144.24.46.16:7777`

---

## One-time installs
- Cursor for Windows
- UE **5.5** + VS2022 (Game development with C++)
- Git + Git LFS + **Git Bash**
- NVIDIA drivers
- Linux cross-compile toolchain + `LINUX_MULTIARCH_ROOT` (for server cook)
- SSH private key at `%USERPROFILE%\.ssh\shadowbanefps_deploy` (from Lehel)

Do **not** install CUDA. Do **not** need OCI tenancy keys.

Engine: `C:\Program Files\Epic Games\UE_5.5`

---

## Exact steps

### 1) Sync `dev`
```powershell
git clone https://github.com/lehelkovach/shadowbanefps.git
cd shadowbanefps
git fetch origin
git checkout dev
git pull origin dev
```

If `dev` is missing:
```powershell
git checkout main
git pull origin main
# if OCI PR not merged yet:
#   git fetch origin cursor/oci-shadowbanefps-server-infra-e09a
#   git checkout cursor/oci-shadowbanefps-server-infra-e09a
git checkout -b dev
git push -u origin dev
```

### 2) Read
- `docs/DEV_WORKFLOW.md` ← **branch + hot deploy**
- `docs/OCI_DEPLOY.md` ← live IP / cook details
- `docs/SETUP.md` §5, `docs/TESTING.md`

### 3) Build Editor
```powershell
$UE = "C:\Program Files\Epic Games\UE_5.5"
$PROJ = "$PWD\ShadowbaneFPS.uproject"
& "$UE\Engine\Build\BatchFiles\Build.bat" -projectfiles -project="$PROJ" -game -engine -progress
& "$UE\Engine\Build\BatchFiles\Build.bat" ShadowbaneFPSEditor Win64 Development -Project="$PROJ" -WaitMutex
```

### 4) Daily hot push (after you commit)
```powershell
git add -A
git commit -m "wip: describe change"
.\scripts\Dev-Push.ps1
.\scripts\Connect-DevServer.ps1
```

`Dev-Push.ps1` will: ensure `dev` → push → cook `LinuxServer` → rsync to
`ubuntu@144.24.46.16` → restart `shadowbanefps-server`.

### 5) Automation
```powershell
.\scripts\RunAutomationTests.ps1
```

---

## Done when
- [x] Editor builds on Gracen's machine
- [ ] On branch `dev` with latest infra/deploy scripts
- [ ] SSH deploy key installed locally
- [ ] `.\scripts\Dev-Push.ps1` succeeds at least once
- [ ] Client connects to **`144.24.46.16:7777`**
- [ ] Failures pasted back to Lehel

## Out of scope
- OCI Terraform / tenancy admin
- Creating VMs
- Committing secrets or `Dist/` builds to git
