// Copyright shadowbanefps.
// Cast invocation facsimile: theurgic AH-TAH (Hebrew Atah / GD "Ateh"), not ripped SB VO.

#pragma once

#include "CoreMinimal.h"

class UObject;
class USoundBase;
class UWorld;

/**
 * Loads Content/Audio/SB_CastShout.wav when present; otherwise synthesizes a short yell.
 * Intent: elongated ritual vibration AAAAH–TAAAAH (Ateh / Atah) — the Shadowbane-adjacent
 * cast shout players remember. Original TTS/procedural stand-in only.
 */
namespace SBCastShout
{
	USoundBase* GetOrCreateShout(UObject* WorldContextObject);
	void PlayAt(UWorld* World, const FVector& Location);
}
