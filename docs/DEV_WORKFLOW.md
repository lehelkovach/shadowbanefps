# DEV branch + hot deploy (simple path)

## Advice (short)

**Yes — do this**, but keep expectations right:

| Want | Reality for Unreal |
| --- | --- |
| Push code → server updates | Needs a **cook** (UE Linux package) then **rsync + systemd restart** |
| True “hot reload” of C++ | **No** — dedicated server restarts with a new build |
| OCI admin on Gracen’s PC | **Not needed** — only an **SSH deploy key** |
| Auto-cook on every `git push` in GitHub cloud | **Not yet** — GitHub-hosted runners don’t have UE 5.5 |

**Simplest model that works now**

1. Branch **`dev`** = continuous playtest (deploy target)  
2. Branch **`main`** = verified baseline (merge from `dev` when good)  
3. Gracen runs **`.\scripts\Dev-Push.ps1`** after commits → pull/`dev` push → cook → deploy to `144.24.46.16`  
4. Optional later: self-hosted GitHub runner on Gracen’s box for real push-to-deploy CI  

Using only `main` also works with 2 people; `dev` just keeps broken mid-work off the “known good” line.

---

## One-time setup (Lehel)

### 1) SSH deploy key (not OCI API)
On a trusted machine:

```bash
ssh-keygen -t ed25519 -f ~/.ssh/shadowbanefps_deploy -N "" -C "shadowbanefps-dev-deploy"
```

Install the **public** key on the VM:

```bash
ssh ubuntu@144.24.46.16 'mkdir -p ~/.ssh && chmod 700 ~/.ssh'
ssh ubuntu@144.24.46.16 'cat >> ~/.ssh/authorized_keys' < ~/.ssh/shadowbanefps_deploy.pub
```

Give Gracen the **private** key file as ` %USERPROFILE%\.ssh\shadowbanefps_deploy `  
(or set `SB_SSH_IDENTITY`). Never commit it. Never put OCI tenancy keys on his PC.

### 2) GitHub secrets (optional Actions)
Repo → Settings → Secrets and variables → Actions:

| Secret | Value |
| --- | --- |
| `DEV_SSH_PRIVATE_KEY` | contents of `shadowbanefps_deploy` private key |
| `DEV_SSH_HOST` | `144.24.46.16` |
| `DEV_SSH_USER` | `ubuntu` |

### 3) Create `dev` branch (once)
After this OCI PR is on `main`:

```bash
git checkout main && git pull
git checkout -b dev
git push -u origin dev
```

---

## Daily loop (Gracen)

```powershell
git checkout dev
git pull origin dev
# ... edit / Cursor agent work ...
.\scripts\Build.ps1 -Target Editor          # or -Target All
.\scripts\Run-Editor.ps1                    # PIE smoke
.\scripts\Run-AdminClient.ps1 -Bots 8       # no humans: spectate bots
# local dedicated (optional):
#   .\scripts\Debug-Local.ps1
git add -A
git commit -m "wip: ..."
.\scripts\Dev-Push.ps1
# cooks LinuxServer + deploys + restarts systemd
.\scripts\Connect-DevServer.ps1   # 144.24.46.16:7777
```

Build/run/debug script map: [`docs/SCRIPTS.md`](./SCRIPTS.md). Bots/admin: [`docs/BOTS_AND_ADMIN.md`](./BOTS_AND_ADMIN.md).

Flags:

```powershell
.\scripts\Dev-Push.ps1 -SkipCook     # redeploy existing Dist\Server\LinuxServer
.\scripts\Dev-Push.ps1 -SkipDeploy   # cook only
.\scripts\Dev-Push.ps1 -NoPush       # don’t git push
```

Requires: UE 5.5, Linux cross-compile toolchain, Git Bash, SSH deploy key.

---

## GitHub Actions

Workflow: [`.github/workflows/deploy-dev.yml`](../.github/workflows/deploy-dev.yml)

- **`workflow_dispatch`**: deploy if a `LinuxServer` artifact was attached; otherwise refresh systemd + restart  
- **`push` to `dev`**: same (useful for unit-file tweaks; full binary still comes from a cook)  

Full auto cook-on-push needs a **self-hosted** Windows runner with UE installed — add later if you want zero local deploy commands.

---

## What stays where

| Role | Has |
| --- | --- |
| Gracen Cursor (local) | UE, cook, SSH deploy key, `Dev-Push.ps1` |
| Lehel / OCI Cloud Agent | OCI admin secrets, Terraform, first VM bring-up |
| GitHub Actions | Optional deploy key only (`DEV_SSH_*`) — not OCI tenancy admin |
