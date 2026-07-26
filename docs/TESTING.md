# Testing & Logging

## Logging

Primary categories (Output Log filter):

| Category | Default | What it covers |
| --- | --- | --- |
| `LogShadowbane` | Log | Match flow, spawns, stages, archetype switches, structure state |
| `LogShadowbaneTelemetry` | Log | Structured pilot telemetry events (design doc §12) |
| `LogShadowbaneCombat` | Warning | Per-hit damage spam (raise to Verbose when debugging combat) |

Configured in `Config/DefaultEngine.ini` under `[Core.Log]`.

### Where to look
- Editor: **Window → Developer Tools → Output Log**, filter `LogShadowbane`
- Packaged / `-game` / dedicated server: `Saved/Logs/ShadowbaneFPS.log`
- Telemetry CSV: `Saved/Telemetry/match_<timestamp>_<session>.csv`

### Useful command-line verbosity
```powershell
# Verbose combat while playtesting
-LogCmds="LogShadowbaneCombat Verbose"
```

## Automation tests

Tests live under `Source/ShadowbaneFPS/Tests/` and are registered under the
`ShadowbaneFPS.*` filter:

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
| `ShadowbaneFPS.Art.PlaceholderIcons` | Dummy icon codes / team color sanity |

### Run from the Editor
1. Build `ShadowbaneFPSEditor`
2. **Tools → Session Frontend → Automation** (or `Window → Developer Tools → Session Frontend`)
3. Filter `ShadowbaneFPS`
4. Start tests

### Run headless (friend's machine / CI later)
PowerShell helper:

```powershell
.\scripts\RunAutomationTests.ps1
```

Or manually (adjust `UE` path):

```powershell
$UE = "C:\Program Files\Epic Games\UE_5.5"
& "$UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "$PWD\ShadowbaneFPS.uproject" `
  -NullRHI -Unattended -NoSound -NoSplash `
  -ExecCmds="Automation RunTests ShadowbaneFPS; Quit"
```

Pass criteria: all `ShadowbaneFPS.*` tests **Success**, process exit code 0.

## What Cloud Agent vs Local Agent can do
- **Cloud Agent (me):** write tests/logging, fix failures from pasted logs
- **Local Agent / friend rig:** actually compile + run the automation suite and PIE
