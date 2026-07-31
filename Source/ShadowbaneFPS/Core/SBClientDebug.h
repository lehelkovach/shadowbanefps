// Copyright shadowbanefps.

#pragma once

#include "CoreMinimal.h"

/** Command-line / console helpers for on-screen client debug messaging. */
struct SHADOWBANEFPS_API FSBClientDebug
{
	/** True when -SBDebug or -SBVerbose is on the command line. */
	static bool IsEnabled();

	/** True when -SBVerbose (extra net/combat detail on HUD). */
	static bool IsVerbose();

	static void PushMessage(const FString& Message, float SecondsToLive = 6.f);
	static void TickMessages(float DeltaSeconds);
	static void GetActiveMessages(TArray<FString>& OutMessages);

private:
	struct FEntry
	{
		FString Text;
		float Remaining = 0.f;
	};

	static TArray<FEntry> Messages;
};
