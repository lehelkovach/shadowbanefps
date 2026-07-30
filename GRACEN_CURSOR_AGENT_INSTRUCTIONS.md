# Gracen's Cursor Agent Instructions

Paste this whole file into **Cursor Agent** on Gracen's Windows gaming PC
(or say: “Follow `GRACEN_CURSOR_AGENT_INSTRUCTIONS.md`”).

You are the **local Unreal client + LinuxServer cook agent** for `shadowbanefps`.  
You have a GPU and UE **5.5**. Cloud Agents cannot compile Unreal — that is your job.  
You do **not** need OCI admin secrets.

---

## Goal (current)
1. Stay on **`main`** (pilot code is merged)  
2. Keep **`ShadowbaneFPSEditor`** building  
3. PIE / local smoke the greybox  
4. Run automation tests  
5. **Cook Linux dedicated server** and (when deploy access exists) push to DEV  
6. Connect client to the live DEV server: **`144.24.46.16:7777`**

---

## One-time installs (if missing)
- Cursor for Windows  
- Epic Games Launcher → **Unreal Engine 5.5**  
- Visual Studio 2022 → **Game development with C++**  
- Git + Git LFS (`git lfs install`)  
- NVIDIA Game Ready/Studio drivers  
- For Linux server cook: UE **Linux cross-compile toolchain** + `LINUX_MULTIARCH_ROOT`  
  (Epic “Linux Development Requirements”)

Do **not** install CUDA. Do **not** need OCI tenancy admin keys.

Default engine path:

`C:\Program Files\Epic Games\UE_5.5`

---

## Exact steps

### 1) Sync `main`
```powershell
git clone https://github.com/lehelkovach/shadowbanefps.git
cd shadowbanefps
git fetch origin
git checkout main
git pull origin main
```

If OCI deploy docs/scripts are only on the open infra PR still:

```powershell
git fetch origin cursor/oci-shadowbanefps-server-infra-e09a
git checkout cursor/oci-shadowbanefps-server-infra-e09a
git pull origin cursor/oci-shadowbanefps-server-infra-e09a
```

Open this folder as the Cursor workspace.

### 2) Read
- `docs/SETUP.md` §5  
- `docs/TESTING.md`  
- `docs/OCI_DEPLOY.md` ← **live IP, cook, deploy, connect**  
- `docs/PLACEHOLDER_ART.md` (optional)

### 3) Build Editor
```powershell
$UE = "C:\Program Files\Epic Games\UE_5.5"
$PROJ = "$PWD\ShadowbaneFPS.uproject"

& "$UE\Engine\Build\BatchFiles\Build.bat" -projectfiles -project="$PROJ" -game -engine -progress
& "$UE\Engine\Build\BatchFiles\Build.bat" ShadowbaneFPSEditor Win64 Development -Project="$PROJ" -WaitMutex
```

Allowlist the engine path (or Run Everything) so UBT isn’t sandboxed out.

### 4) Local smoke (PIE)
```powershell
& "$UE\Engine\Binaries\Win64\UnrealEditor.exe" "$PROJ"
```

Expect greybox Broken Citadel, team colors, HUD chips, world markers.

### 5) Automation
```powershell
.\scripts\RunAutomationTests.ps1
```

### 6) Cook Linux dedicated server (DEV package)
Requires Linux cross-compile toolchain installed for UE 5.5.

```powershell
.\scripts\Cook-LinuxServer.ps1
# or see docs/OCI_DEPLOY.md §5 (RunUAT BuildCookRun … -platform=Linux -server -noclient)
```

Output should look like:

```text
Dist/Server/LinuxServer/
  ShadowbaneFPSServer.sh
  ...
```

### 7) Connect to live DEV server
```powershell
.\scripts\Connect-DevServer.ps1
# → 144.24.46.16:7777
```

**Note:** systemd is up, but until a real `LinuxServer/` cook is deployed the
server may still be a placeholder — report connect/journal errors if it fails.

### 8) Deploy cook (only if you have SSH to the VM)
Lehel / OCI agent owns infra keys. If you were given an SSH key that the VM
trusts:

```powershell
# Git Bash / WSL:
export SB_DEV_IP=144.24.46.16
export SB_SSH_USER=ubuntu
export SB_SSH_IDENTITY=/path/to/your_key
./scripts/deploy-server.sh --target dev --src Dist/Server/LinuxServer
```

Otherwise: upload `Dist/Server/LinuxServer` to Lehel / Cloud Agent and ask them
to run `deploy-server.sh`.

---

## Done when
- [x] Editor build succeeded on Gracen's machine  
- [ ] On latest `main` (or OCI infra branch if not merged yet)  
- [ ] PIE greybox OK  
- [ ] Automation `ShadowbaneFPS.*` green (or failures pasted)  
- [ ] LinuxServer cook produced under `Dist/Server/LinuxServer`  
- [ ] Client launch attempted against **`144.24.46.16:7777`**  
- [ ] Errors / logs pasted back to Lehel  

## Out of scope for Gracen
- OCI tenancy admin / Terraform apply  
- Creating/destroying VMs  
- Real art production  
- Writing secrets into git  
