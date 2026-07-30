# Local build / run / debug scripts (Windows + UE 5.5)
#
# Prefer these over raw `Build.bat` / `UnrealEditor.exe` calls.
# Set `UE_ROOT` (or pass `-EngineRoot`) if the engine is not under
# `C:\Program Files\Epic Games\UE_5.5`.

## Quick map

| Script | What it does |
| --- | --- |
| `Build.ps1` | Compile Editor / Game / Server / All |
| `Run-Editor.ps1` | Launch Unreal Editor (PIE) |
| `Run-Game.ps1` | Standalone client (`-game`), optional `-Server` |
| `Run-Server.ps1` | Local Win64 dedicated server |
| `Debug-Editor.ps1` | Build + Editor with verbose `LogShadowbane*` |
| `Debug-Local.ps1` | Build All + local server + one client |
| `Run-AdminClient.ps1` | Spectator + `?Bots=N` populate (no humans) |
| `Open-VS.ps1` | Generate `.sln` + open Visual Studio (F5 / Attach) |
| `RunAutomationTests.ps1` | Headless `ShadowbaneFPS.*` tests |
| `RunClientTests.ps1` / `RunServerTests.ps1` / `RunIntegrationTests.ps1` | Suite filters |
| `Connect-DevServer.ps1` | Client → live OCI DEV (`144.24.46.16:7777`) |
| `Cook-LinuxServer.ps1` / `Dev-Push.ps1` | Linux cook + hot deploy |

Shared helpers: `scripts/lib/UeCommon.ps1`.

## Daily local loop

```powershell
git checkout dev
git pull origin dev

.\scripts\Build.ps1 -Target Editor -GenerateProjectFiles
.\scripts\Run-Editor.ps1

# or one-shot with verbose logs
.\scripts\Debug-Editor.ps1
```

## Local dedicated (no OCI)

```powershell
.\scripts\Build.ps1 -Target All
.\scripts\Run-Server.ps1 -Port 7777
.\scripts\Run-Game.ps1 -Server 127.0.0.1:7777

# or
.\scripts\Debug-Local.ps1
```

## Bot populate + admin spectate

```powershell
.\scripts\Run-AdminClient.ps1 -Bots 8
# Console: AddBots 10 / AdminSpectate
```

See [`BOTS_AND_ADMIN.md`](./BOTS_AND_ADMIN.md).

## Breakpoints (Visual Studio)

```powershell
.\scripts\Open-VS.ps1 -GenerateProjectFiles -Build
# Option A: set startup project ShadowbaneFPSEditor → F5
# Option B: .\scripts\Run-Editor.ps1  then Debug → Attach → UnrealEditor.exe
# Option C: .\scripts\Run-Server.ps1  then Attach → ShadowbaneFPSServer.exe
```

Use **Development** (default) for symbols. `DebugGame` is available via `-Config DebugGame` when you want stricter debug runtime checks.

Log filters in Output Log / `Saved/Logs/`: `LogShadowbaneServer`, `LogShadowbaneClient`, `LogShadowbaneNet`.

## Live DEV server

```powershell
.\scripts\Dev-Push.ps1              # cook LinuxServer + rsync + restart
.\scripts\Connect-DevServer.ps1     # or Run-Game.ps1 -Server 144.24.46.16:7777
```

See [`DEV_WORKFLOW.md`](./DEV_WORKFLOW.md) and [`TESTING.md`](./TESTING.md).
