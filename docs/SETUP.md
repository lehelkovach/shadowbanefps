# Setup & Operations — Conquest Siege Pilot

This is the practical build/run guide for the pilot described in
[`docs/game-design.md`](./game-design.md). It is split into **what you do on your
machines**, **what the agent does in the repo**, and **the dedicated-server (OCI)
plan**.

> Engine target: **Unreal Engine 5.5** (set in `ShadowbaneFPS.uproject` →
> `EngineAssociation`). If you standardize on a different 5.x, change that field
> and the `IncludeOrderVersion` lines in the three `Source/*.Target.cs` files.

---

## 1. Roles of your two machines

| Machine | GPU | Recommended role |
| --- | --- | --- |
| Lenovo Yoga 7i | Intel Iris Xe (integrated) | Code/blueprint editing, git, docs, light iteration. The Unreal Editor *runs* but real-time gameplay will be rough. Fine as a second client for netcode testing. |
| MSI gaming laptop | Dedicated NVIDIA GPU | **Primary dev + client test machine.** Install the full engine, compile C++, run the editor, package the game client, and drive playtests here. |

You do **not** need a beefy GPU for the *server* — the dedicated server is
headless. That's what the OCI VM is for (section 4).

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

## 5. Quick start checklist

- [ ] Link Epic ↔ GitHub; install UE 5.5 on the MSI laptop.
- [ ] Install VS2022 (C++ game dev), Git, Git LFS (`git lfs install`).
- [ ] Clone repo; generate VS project files; build `Development Editor`.
- [ ] Open `ShadowbaneFPS.uproject`; confirm it loads with the pilot GameMode.
- [ ] (Server) install the UE Linux cross-compile toolchain.
- [ ] Create the OCI VM (section 4) and share access.
- [ ] Tell me: engine version confirmed? OCI shape/region? x86 or ARM?
