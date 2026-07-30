# Setup & Operations — Conquest Siege Pilot

This is the practical build/run guide for the pilot described in
[`docs/game-design.md`](./game-design.md). It is split into **what you do on your
machines**, **what the agent does in the repo**, and **the dedicated-server (OCI)
plan**.

> Engine target: **Unreal Engine 5.5** (set in `ShadowbaneFPS.uproject` →
> `EngineAssociation`). If you standardize on a different 5.x, change that field
> and the `IncludeOrderVersion` lines in the three `Source/*.Target.cs` files.

---

## 1. Roles of the machines

| Machine | GPU | Recommended role |
| --- | --- | --- |
| Lenovo Yoga 7i | Intel Arc (integrated) | Code/docs, git, light iteration, **local headless server**, second low-settings client. |
| MSI gaming laptop | Dedicated NVIDIA GPU | Primary personal client/editor machine. |
| Friend's gaming rig | Strong dedicated GPU | **Best place for Local Cursor Agent + client build/playtest.** This is where Unreal Editor and PIE should live. |

You do **not** need a beefy GPU for the *server* — the dedicated server is
headless. That's what the OCI VM is for (section 4), or a local headless
process on the Yoga / friend's box.

> **Cloud Agent (me) vs Local Agent (friend's Cursor Desktop):** I write code and
> open PRs from a cloud VM with **no GPU / no Unreal install**. Your friend's
> machine runs **Cursor Desktop → Agent**, which can drive the real local
> toolchain (compile, launch editor/game). That is the right way to build and
> test the Unreal *client*.

---

## 2. What you do on your end (accounts + local setup)

### 2.1 Epic / Unreal account (required, free)
1. Create an Epic Games account and accept the Unreal Engine EULA.
2. **Link your GitHub account to Epic** at <https://www.unrealengine.com/en-US/ue-on-github>.
   This grants access to the Unreal Engine source repo, which you need if you
   want to build the **Linux dedicated server** from source (the pre-built
   launcher engine does not ship the Linux server toolchain on Windows).
   - You do *not* need to give me your Epic credentials. Account linking is a
     one-time thing you do in a browser.
3. Install the Epic Games Launcher on the MSI laptop and install **UE 5.5**.

### 2.2 Toolchain on the MSI laptop (primary)
- **Visual Studio 2022** with the "Game development with C++" workload (and the
  ".NET desktop" + MSVC toolset). Required to compile the C++ modules in `Source/`.
- **Git** + **Git LFS**: run `git lfs install` once. Our `.gitattributes`
  already routes `*.uasset`, `*.umap`, textures, audio, meshes, etc. through LFS.
- Clone this repo, then right-click `ShadowbaneFPS.uproject` →
  **Generate Visual Studio project files** → open the `.sln` → build
  `Development Editor | Win64`.

### 2.3 To build a Linux dedicated server from a Windows host
- In the Epic Launcher, on UE 5.5, install the **Linux cross-compile toolchain**
  (Epic ships a clang toolchain; set the `LINUX_MULTIARCH_ROOT` env var per Epic's
  "Linux Development Requirements" docs). This lets the MSI laptop cook/package a
  Linux server build that runs on the OCI VM. See section 4.

### 2.4 What I need from you (decisions / access)
- **Confirm the engine version** (I defaulted to 5.5). If you already installed a
  specific 5.x, tell me and I'll align the project files.
- **OCI:** yes please — see section 4 for the exact shape (region, shape/size,
  ports). If you can create the VM and give me SSH access (or run the deploy
  script I provide), I can wire up the server-run + CI side.
- **I do NOT need your Epic password or personal credentials.** Account linking
  and launcher installs are things only you can/should do in a browser on your
  machines. If we later use a CI service to build the engine, we'll use a
  dedicated service account + token, added via Cursor Dashboard → Cloud Agents →
  Secrets (never pasted into chat).

---

## 3. What the agent (me) does in this repo

Already scaffolded in this PR:
- Project skeleton: `ShadowbaneFPS.uproject`, `Config/*.ini`, and the three build
  targets (`Game`, `Editor`, **`Server`** for the dedicated server).
- Repo hygiene: `.gitignore` (UE-generated folders) and `.gitattributes` (Git LFS
  for binary art) so the repo stays clean across Windows + Linux.
- A server-authoritative C++ framework that maps 1:1 onto the design doc:
  - `SBSiegeGameMode` — match flow: teams, 20-min clock, phase pacing (§8),
    conquest-stage advance, final-objective win, **overtime** (§5), respawn +
    post-death archetype switching with duplicate/side/spawn rules (§9).
  - `SBSiegeGameState` — replicated HUD state: timer, phase, conquest stage,
    final-objective progress, result (§5, §8, §11).
  - `SBPlayerState` — team + selected pre-built character + alive flag, with a
    documented hidden-composition disclosure model (§4).
  - `SBCharacterArchetype` (data asset) — a fully pre-built roster character with
    Shadowbane descriptors + a lobby-readable role profile + duplicate limits (§3).
  - `SBDestructibleStructure` — intact/damaged/destroyed gate/wall/emplacement
    with repair rules (§7).
  - `SBCapturePoint` — the courtyard conquest zone that advances the front line (§7).
  - `SBConquestObjective` — the interruptible inner-keep final objective feeding
    progress + overtime (§5).

What I can do next (say the word / prioritize):
- Character pawn + GameplayAbilitySystem base classes and 8–12 concrete roster
  archetypes (§3).
- Battlefield-intel system: pings, short-lived enemy markers, death recap, and the
  per-connection replication that actually *hides* enemy composition (§4, §11).
- Spawn manager: staged attacker/defender spawns + forward-spawn relocation on
  courtyard capture (§7, §9).
- HUD/UMG widgets for timer, conquest stage, structure health, objective progress.
- Telemetry sink writing the §12 metrics to a log/CSV/analytics endpoint.
- A greybox `BrokenCitadel` blockout (three routes, gate + alt breach, courtyard,
  inner keep) — I can generate this procedurally/geometry-scripted since I can't
  hand-author in-editor.
- CI (GitHub Actions) that compiles the code + optionally cooks the Linux server.

> I can't run the Unreal Editor or compile UE here (no GPU / engine install in the
> agent VM). So I write idiomatic UE5 C++ and configs, and **you compile on the
> MSI laptop**. If a compile error shows up, paste it and I'll fix it fast.

---

## 4. Dedicated server on OCI (Oracle Cloud)

The pilot is small (10 players), so the server is cheap and can even fit Oracle's
Always-Free tier for early testing.

### 4.1 Recommended VM shape
- **Early testing (free):** `VM.Standard.A1.Flex` (Ampere/ARM, Always-Free:
  up to 4 OCPU / 24 GB). Note: an **ARM** server build requires cooking for
  `LinuxArm64`. Simplest cross-compile path is x86, so for least friction use:
- **Recommended:** `VM.Standard.E4.Flex` or `E5.Flex`, **2 OCPU / 8 GB RAM**,
  Ubuntu 22.04 LTS (x86_64). A 5v5 UE dedicated server is CPU-light; 2 vCPU is
  ample for the pilot, scale later.
- Boot volume: 50 GB is plenty for one packaged server build + logs.

### 4.2 Networking (OCI security list / NSG)
Open inbound on the VCN + the VM firewall:
- **UDP 7777** — Unreal game traffic (default `GameNetDriver` port).
- **UDP 7778** (optional) — a second instance / beacon if you run two matches.
- **TCP 22** — SSH (restrict to your IPs).
- If you later add a query/stats port or RCON-style admin, open those explicitly.

### 4.3 Deploy flow (once you build a Linux server package)
On the MSI laptop (cross-compiling to Linux):
```
RunUAT BuildCookRun -project=ShadowbaneFPS.uproject -noP4 ^
  -platform=Linux -serverconfig=Development -server -noclient ^
  -cook -stage -pak -archive -archivedirectory=Dist/Server
```
Copy the archived `LinuxServer/` folder to the VM and run:
```
./ShadowbaneFPSServer.sh BrokenCitadel -log -port=7777
```
I'll provide a `systemd` unit + a `deploy.sh` (rsync + restart) so updates are one
command. Give me SSH access or run the scripts yourself — either works.

### 4.4 What I need from you to wire this up
- OCI region + a created VM (or permission/keys to create/configure one).
- The VM's public IP and an SSH key I can use (add the private key via Cursor
  Dashboard → Secrets, or you run my deploy script and paste the output).
- Confirmation of x86 vs ARM so I target the right Linux cook.

---

## 5. Friend / collaborator onboarding (Local Cursor Agent)

This is how your friend's gaming rig becomes a real Unreal client build+test
station. **Yes — Cursor still does local dev.** Cursor Desktop is local-first;
Cloud Agents are an optional extra. Friend uses Desktop Agent on their PC.

### 5.1 One-time: give them repo access
1. On GitHub (`lehelkovach/shadowbanefps`), **Settings → Collaborators → Add**
   their GitHub username (Write access).
2. They accept the invite in email / GitHub.
3. They install [Cursor Desktop](https://cursor.com/download) and sign in with
   *their own* Cursor account (they do **not** need yours).

### 5.2 One-time: install the Unreal toolchain on their PC
Same stack as section 2 — friend needs **their own** Epic account:

1. Epic Games account → accept Unreal EULA → install Epic Launcher → install
   **UE 5.5** (must match `EngineAssociation` in `ShadowbaneFPS.uproject`).
2. Optional but recommended for server packaging later: link Epic ↔ GitHub at
   <https://www.unrealengine.com/en-US/ue-on-github>.
3. **Visual Studio 2022** with workload **"Game development with C++"**
   (MSVC + .NET desktop components).
4. **Git** + **Git LFS**: `git lfs install`
5. Clone and open in Cursor:
   ```powershell
   git clone https://github.com/lehelkovach/shadowbanefps.git
   cd shadowbanefps
   git checkout cursor/ue5-conquest-siege-pilot-scaffold-e6f0
   cursor .
   ```
6. Free disk: UE + Intermediate/DerivedDataCache wants **~80–150+ GB** free.
   32 GB RAM is comfortable; 16 GB works but will page.

### 5.3 Cursor Agent settings that matter for Unreal builds
UnrealBuildTool lives **outside** the repo (under the Epic install), so the
default Agent sandbox often can't see it.

On the friend's machine, in Cursor Agent:

- Prefer **Allowlist** (or temporarily **Run Everything**) for build nights —
  Auto-review will otherwise spam approval prompts on every UBT invocation.
  Docs: [Run Modes](https://cursor.com/docs/agent/security/run-modes).
- Allow terminal access to the engine tree (example path — adjust to their
  install), e.g. via `~/.cursor/sandbox.json` / permissions allowlist:
  - `C:\Program Files\Epic Games\UE_5.5\**`
  - Network if they use marketplace/LFS remotes.
- First compile can take **30–90+ minutes**. Keep Cursor open; don't treat it
  like a 30s script. If the Agent stalls waiting for approval mid-build, click
  Allow / add to allowlist and re-prompt.

Local Agent **can**: edit code, run shell commands (generate project files,
  compile, launch `UnrealEditor.exe` / packaged game), read build logs, fix
  compile errors, open a browser.
Local Agent **cannot** (today): fully drive the Unreal Editor UI like a human
  (viewport clicking, Blueprint node wiring by mouse). Treat PIE / feel-testing
  as a **human** job; treat compile + launch + log triage as an **Agent** job.

### 5.4 Exact build + launch commands (Windows)

Set `UE` to their engine root once per shell:

```powershell
$UE = "C:\Program Files\Epic Games\UE_5.5"
$PROJ = "$PWD\ShadowbaneFPS.uproject"
```

**Generate project files** (first clone, or after adding C++ files):
```powershell
& "$UE\Engine\Build\BatchFiles\Build.bat" -projectfiles -project="$PROJ" -game -engine -progress
```

**Compile Development Editor** (what you need to open the project):
```powershell
& "$UE\Engine\Build\BatchFiles\Build.bat" ShadowbaneFPSEditor Win64 Development -Project="$PROJ" -WaitMutex
```

**Launch the editor** (human does PIE / map work after it opens):
```powershell
& "$UE\Engine\Binaries\Win64\UnrealEditor.exe" "$PROJ"
```

**Launch a standalone game client** (no editor chrome — good for playtest):
```powershell
& "$UE\Engine\Binaries\Win64\UnrealEditor.exe" "$PROJ" -game -windowed -ResX=1920 -ResY=1080 -log
```

**Local listen-server smoke** (one process hosts, second joins — early netcheck):
```powershell
# Terminal A — host
& "$UE\Engine\Binaries\Win64\UnrealEditor.exe" "$PROJ" -game -log

# Terminal B — client join (after host is up)
& "$UE\Engine\Binaries\Win64\UnrealEditor.exe" "$PROJ" 127.0.0.1:7777 -game -log
```

Once an OCI dedicated server exists (section 4), clients join with the VM IP:
```powershell
& "$UE\Engine\Binaries\Win64\UnrealEditor.exe" "$PROJ" YOUR.OCI.IP:7777 -game -log
```

### 5.5 Prompt the friend's Local Agent with this

Paste something like this into **Cursor Desktop → Agent** on their rig:

> You are on the Unreal client machine for `shadowbanefps`.
> Engine is UE 5.5 at `C:\Program Files\Epic Games\UE_5.5`.
> Read `docs/SETUP.md` §5 and `docs/game-design.md`.
> 1) Confirm the engine path exists.
> 2) Generate project files, then build `ShadowbaneFPSEditor` Win64 Development.
> 3) If the build fails, fix C++ compile errors and rebuild until it succeeds.
> 4) Launch the editor with `ShadowbaneFPS.uproject`.
> 5) Summarize any missing assets/maps (expect `BrokenCitadel` to be missing
>    until we greybox it) and paste the first fatal log lines.
> Do not commit secrets. Prefer a feature branch named `cursor/<thing>-e6f0`.

### 5.6 Collaboration rules (so you don't step on each other)
- **One owner per concern per day:** friend's rig owns *client build / PIE /
  feel*; Cloud Agent (me) owns scaffolding/PRs; Yoga can own docs + local
  dedicated-server experiments.
- Work on **feature branches**, not straight to `main`. Pull before you push.
- Don't both edit the same C++ files blind — if both need to touch GameMode,
  coordinate or serialize PRs.
- Binary assets (`*.uasset` / `*.umap`) go through **Git LFS**. Only one person
  should author a given asset at a time (UE merge conflicts on assets are pain).
- Compile errors / crash logs: paste into chat or open an issue — I can fix
  from the cloud side fast; friend rebuilds locally.

### 5.7 What "done" looks like for their first session
- [ ] Repo cloned on friend's PC; Cursor Desktop opens the project folder.
- [ ] UE 5.5 + VS2022 installed; Agent (or human) generated project files.
- [ ] `ShadowbaneFPSEditor` Win64 Development **builds successfully**.
- [ ] Editor launches on `/Engine/Maps/Entry` — the GameMode auto-spawns the
      **Broken Citadel greybox** (no custom `.umap` required).
- [ ] PIE / `-game`: you spawn as attacker or defender, see the debug HUD, can
      shoot, damage the gate, capture the courtyard, and switch characters after death.
- [ ] Any compile errors they hit are pasted back so Cloud Agent can patch them.

### 5.8 In-game controls (current greybox)
| Input | Action |
| --- | --- |
| WASD | Move |
| Mouse | Look |
| Space | Jump |
| LMB | Fire (hitscan; damages players + structures) |
| F | Ping (debug marker) |
| E | Interact (reserved) |
| 1–0 | Select pre-built character (while dead) |
| R | Request respawn |

---

## 6. Quick start checklist

- [ ] Link Epic ↔ GitHub; install UE 5.5 on at least one strong GPU machine
      (MSI and/or friend's rig).
- [ ] Install VS2022 (C++ game dev), Git, Git LFS (`git lfs install`).
- [ ] Add friend as GitHub collaborator; they clone and open in Cursor Desktop.
- [ ] Friend's Local Agent builds `ShadowbaneFPSEditor` (section 5).
- [ ] Open `ShadowbaneFPS.uproject`; confirm it loads with the pilot GameMode.
- [ ] Run automation: `.\scripts\RunAutomationTests.ps1` (see `docs/TESTING.md`).
- [ ] (Server) install the UE Linux cross-compile toolchain when ready.
- [ ] Create the OCI VM (section 4) and share access.
- [ ] Tell me: engine version confirmed? OCI shape/region? x86 or ARM?
