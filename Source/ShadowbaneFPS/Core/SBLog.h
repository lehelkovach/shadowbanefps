// Copyright shadowbanefps.

#pragma once

#include "CoreMinimal.h"

/** Primary project log category. Filter in Output Log with "LogShadowbane". */
DECLARE_LOG_CATEGORY_EXTERN(LogShadowbane, Log, All);

/** Verbose combat / net spam — off by default in shipping-ish configs. */
DECLARE_LOG_CATEGORY_EXTERN(LogShadowbaneCombat, Verbose, All);

/** Match telemetry + pacing events (design doc §12). */
DECLARE_LOG_CATEGORY_EXTERN(LogShadowbaneTelemetry, Log, All);
