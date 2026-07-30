# Gracen's Cursor Agent Instructions

Paste this whole file into **Cursor Agent** on Gracen's Windows gaming PC (or say: “Follow `GRACEN_CURSOR_AGENT_INSTRUCTIONS.md`”).

You are the **local Unreal client build + test agent** for `shadowbanefps`.  
You have a GPU and (or will install) UE 5.5. The Cloud Agent cannot compile Unreal — that is your job.

---

## Goal
1. Get the pilot branch locally  
2. Build **`ShadowbaneFPSEditor`** (UE 5.5, Win64 Development)  
3. Launch PIE / editor and confirm the greybox runs  
4. Run automation tests  
5. Report success or paste compile/run errors back to the team

---

## One-time installs (do these if missing)
- Cursor for Windows
- Epic Games Launcher → **Unreal Engine 5.5**
- Visual Studio 2022 with workload **Game development with C++**
- Git + Git LFS (`git lfs install`)
- NVIDIA Game Ready/Studio drivers  

Do **not** install CUDA toolkit. Do **not** need OCI secrets for this task.

Engine path default (adjust if different):

`C:\Program Files\Epic Games\UE_5.5`

---

## Exact steps

### 1) Clone / update and checkout the pilot branch
```powershell
git clone https://github.com/lehelkovach/shadowbanefps.git
cd shadowbanefps
git fetch origin
git checkout cursor/ue5-conquest-siege-pilot-scaffold-e6f0
git pull origin cursor/ue5-conquest-siege-pilot-scaffold-e6f0
```

If the repo is already cloned, just `cd` into it and run the fetch/checkout/pull lines.

Open this folder as the Cursor workspace.

### 2) Read the ops docs on that branch
- `docs/SETUP.md` §5 (local Cursor Agent / friend onboarding)
- `docs/TESTING.md`
- `docs/PLACEHOLDER_ART.md` (optional)

### 3) Generate project files + build ShadowbaneFPSEditor
```powershell
$UE = "C:\Program Files\Epic Games\UE_5.5"
$PROJ = "$PWD\ShadowbaneFPS.uproject"

& "$UE\Engine\Build\BatchFiles\Build.bat" -projectfiles -project="$PROJ" -game -engine -progress

& "$UE\Engine\Build\BatchFiles\Build.bat" ShadowbaneFPSEditor Win64 Development -Project="$PROJ" -WaitMutex
```

Cursor Agent settings tip: allow the engine path outside the repo (or use Allowlist / Run Everything) so UnrealBuildTool is not sandboxed away.

### 4) Launch and smoke-test
```powershell
& "$UE\Engine\Binaries\Win64\UnrealEditor.exe" "$PROJ"
```

Or PIE from the editor. Expect:
- Greybox **Broken Citadel** (runtime-built; no custom `.umap` required)
- Team-colored pawn + floating rune disc
- HUD with timer / archetype chips / dummy ability runes
- World markers: MAIN GATE, COURTYARD, KEEP RUNE, routes

Controls: WASD move, mouse look, LMB fire, `1-0` switch character while dead, `R` respawn.

### 5) Run automation tests
```powershell
.\scripts\RunAutomationTests.ps1
```

(or Session Frontend → Automation → filter `ShadowbaneFPS`)

---

## Done when
- [x] On branch `cursor/ue5-conquest-siege-pilot-scaffold-e6f0`
- [x] `ShadowbaneFPSEditor` Win64 Development build succeeded *(Gracen — verified)*
- [ ] Editor/PIE launches and greybox is visible
- [ ] Automation `ShadowbaneFPS.*` is green (or failures pasted)
- [ ] Any errors are copied back to Lehel / the Cloud Agent chat

## Out of scope for Gracen right now
- OCI / cloud server admin secrets
- Linux dedicated server packaging
- Real art production
- Merging to `main` (Lehel does that after your green build — **build is done; merge is next**)
