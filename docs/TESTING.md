# Testing & Logging

## Logging

Primary categories (Output Log filter):

| Category | Default | What it covers |
| --- | --- | --- |
| `LogShadowbane` | Log | General match flow (mirrors key server events) |
| `LogShadowbaneServer` | Log | Dedicated / authority: InitGame, spawn, kill, capture, phase, match end |
| `LogShadowbaneClient` | Log | HUD ready, death/respawn UX, OnRep reactions, local input |
| `LogShadowbaneNet` | Log | Replication / RPC boundaries (phase, stage, capture, structure, archetype, respawn) |
| `LogShadowbaneTelemetry` | Log | Structured pilot telemetry events (design doc §12) |
| `LogShadowbaneCombat` | Warning | Per-hit damage spam (raise to Verbose when debugging combat) |

Configured in `Config/DefaultEngine.ini` under `[Core.Log]`.

### Where to look
- Editor: **Window → Developer Tools → Output Log**, filter `LogShadowbane` (or `Server` / `Client` / `Net`)
- Packaged client / `-game`: `Saved/Logs/ShadowbaneFPS.log`
- Dedicated server (OCI DEV): `journalctl -u shadowbanefps-server -f` and/or the cooked server's `Saved/Logs/`
- Telemetry CSV: `Saved/Telemetry/match_*.csv` (flow) and `combat_*.csv` (build-vs-build damage/heals/kills for balance)

### Useful command-line verbosity
```powershell
# Verbose combat + net while playtesting
-LogCmds="LogShadowbaneCombat Verbose,LogShadowbaneNet Verbose,LogShadowbaneClient Verbose"
```

On the dedicated server you can also set env / DefaultEngine overrides so
`LogShadowbaneServer` stays at `Log` and combat stays quiet.

## Automation tests

Tests live under `Source/ShadowbaneFPS/Tests/` and are registered under the
`ShadowbaneFPS.*` filter. Suites:

| Filter | Focus |
| --- | --- |
| `ShadowbaneFPS.Rules.*` | Pure match rules (structure, conquest, caps, overtime) |
| `ShadowbaneFPS.Roster.*` / `Spawn.*` / `Telemetry.*` / `Art.*` | Roster shape, spawn matrix, telemetry, placeholders |
| `ShadowbaneFPS.Server.*` | Server capture/objective ticks, friendly-fire, team fill |
| `ShadowbaneFPS.Client.*` | HUD clock, death overlay, respawn/switch UX |
| `ShadowbaneFPS.Integration.*` | End-to-end rule chains (match flow, OT, archetype policy, front line) |

| Test name | Covers |
| --- | --- |
| `ShadowbaneFPS.Rules.StructureState` | Intact / Damaged / Destroyed thresholds |
| `ShadowbaneFPS.Rules.ConquestAdvance` | Front line only moves forward |
| `ShadowbaneFPS.Rules.DuplicateLimit` | Per-team archetype caps |
| `ShadowbaneFPS.Rules.TeamBalance` | Joiners fill the smaller side |
| `ShadowbaneFPS.Rules.ObjectiveAndOvertime` | Final objective unlock + overtime gate |
| `ShadowbaneFPS.Roster.DefaultShape` | 8–12 unique pre-builts, side eligibility |
| `ShadowbaneFPS.Roster.RoleCoverage` | Healer / siege / detection / control present |
| `ShadowbaneFPS.Spawn.AvailabilityMatrix` | Staged spawn availability by team/stage |
| `ShadowbaneFPS.Telemetry.SessionLifecycle` | Telemetry session start/events/end |
| `ShadowbaneFPS.Telemetry.CombatBalanceAttribution` | Damage/kill attribution by build + power; balance summary |
| `ShadowbaneFPS.Art.PlaceholderIcons` | Dummy icon codes / team color sanity |
| `ShadowbaneFPS.Server.CaptureTick` | Capture advance / contest / decay |
| `ShadowbaneFPS.Server.ObjectiveTick` | Final objective channel math |
| `ShadowbaneFPS.Server.FriendlyFireGate` | Same-team damage blocked |
| `ShadowbaneFPS.Server.TeamBalanceJoin` | 5v5 fill order |
| `ShadowbaneFPS.Client.HudClock` | `MM:SS` formatting |
| `ShadowbaneFPS.Client.DeathOverlay` | When death UX should show |
| `ShadowbaneFPS.Client.RespawnAndSwitchUx` | Dead-only switch + respawn gate |
| `ShadowbaneFPS.Client.ObjectiveHudProgress` | 0..1 progress for HUD bar |
| `ShadowbaneFPS.Integration.MatchFlowPath` | Outer → courtyard → keep → objective complete |
| `ShadowbaneFPS.Integration.OvertimeGate` | Contested OT window |
| `ShadowbaneFPS.Integration.ArchetypeSwitchPolicy` | Dead switch + duplicate limits + sides |
| `ShadowbaneFPS.Integration.SpawnFrontLine` | No retreat + gate structure path |

### Run from the Editor
1. Build `ShadowbaneFPSEditor`
2. **Tools → Session Frontend → Automation** (or `Window → Developer Tools → Session Frontend`)
3. Filter `ShadowbaneFPS` (or `ShadowbaneFPS.Client` / `.Server` / `.Integration`)
4. Start tests

### Run headless (friend's machine / CI later)
```powershell
# Everything
.\scripts\RunAutomationTests.ps1

# Suites
.\scripts\RunClientTests.ps1
.\scripts\RunServerTests.ps1
.\scripts\RunIntegrationTests.ps1
```

Build / run / debug helpers (Editor, local dedicated, VS attach): see [`docs/SCRIPTS.md`](./SCRIPTS.md).

Or manually (adjust `UE` path):

```powershell
$UE = "C:\Program Files\Epic Games\UE_5.5"
& "$UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "$PWD\ShadowbaneFPS.uproject" `
  -NullRHI -Unattended -NoSound -NoSplash `
  -ExecCmds="Automation RunTests ShadowbaneFPS; Quit"
```

Pass criteria: all selected `ShadowbaneFPS.*` tests **Success**, process exit code 0.

## What Cloud Agent vs Local Agent can do
- **Cloud Agent:** write tests/logging, fix failures from pasted logs (no UE binary here)
- **Local Agent / Gracen rig:** compile + run the automation suites, PIE, and dedicated-server cook
- After a cook/deploy, confirm server logs with `LogShadowbaneServer` / `LogShadowbaneNet` on the VM
