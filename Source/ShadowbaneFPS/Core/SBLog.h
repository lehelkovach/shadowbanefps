// Copyright shadowbanefps.

#pragma once

#include "CoreMinimal.h"

/** Primary project log category. Filter in Output Log with "LogShadowbane". */
DECLARE_LOG_CATEGORY_EXTERN(LogShadowbane, Log, All);

/** Server-authoritative match flow (GameMode, capture, spawn, kills). */
DECLARE_LOG_CATEGORY_EXTERN(LogShadowbaneServer, Log, All);

/** Client-facing UX (HUD, input, local OnRep reactions). */
DECLARE_LOG_CATEGORY_EXTERN(LogShadowbaneClient, Log, All);

/** Replication / RPC boundaries (phase/stage reps, archetype & respawn RPCs). */
DECLARE_LOG_CATEGORY_EXTERN(LogShadowbaneNet, Log, All);

/** Verbose combat / net spam — off by default in shipping-ish configs. */
DECLARE_LOG_CATEGORY_EXTERN(LogShadowbaneCombat, Verbose, All);

/** Match telemetry + pacing events (design doc §12). */
DECLARE_LOG_CATEGORY_EXTERN(LogShadowbaneTelemetry, Log, All);
