// Copyright shadowbanefps.

#pragma once

#include "CoreMinimal.h"

class UWorld;

/** Lightweight bridge used by replicated combat events to reach each local HUD. */
namespace SBDamageFloat
{
	void Push(UWorld* World, const FVector& WorldLocation, float Amount, bool bHeal);
}
