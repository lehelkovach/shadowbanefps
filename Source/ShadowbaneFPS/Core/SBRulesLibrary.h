// Copyright shadowbanefps.
//
// Pure helpers for match rules so automation tests can cover them without PIE.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SBTypes.h"
#include "SBRulesLibrary.generated.h"

UCLASS()
class SHADOWBANEFPS_API USBRulesLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Maps health fraction + damaged threshold to intact/damaged/destroyed. */
	UFUNCTION(BlueprintPure, Category = "Siege|Rules")
	static ESBStructureState ComputeStructureState(float HealthPercent, float DamagedThreshold = 0.5f);

	/** True if Next is the same stage or forward-only (pilot never retreats the front line). */
	UFUNCTION(BlueprintPure, Category = "Siege|Rules")
	static bool CanAdvanceConquestStage(ESBConquestStage Current, ESBConquestStage Next);

	/** Duplicate-limit check. Limit <= 0 means unlimited. */
	UFUNCTION(BlueprintPure, Category = "Siege|Rules")
	static bool CanUseArchetypeSlot(int32 CurrentTeamCount, int32 PerTeamDuplicateLimit);

	/** Balance joiners onto the smaller side; ties go to Attackers. */
	UFUNCTION(BlueprintPure, Category = "Siege|Rules")
	static ESBTeam PickBalancedTeam(int32 AttackerCount, int32 DefenderCount);

	/** Final objective unlocks once the interior is in play (not OuterSiege). */
	UFUNCTION(BlueprintPure, Category = "Siege|Rules")
	static bool IsFinalObjectiveUnlocked(ESBConquestStage Stage);

	/** Regulation overtime should start when the clock hits zero with in-progress objective. */
	UFUNCTION(BlueprintPure, Category = "Siege|Rules")
	static bool ShouldEnterOvertime(float FinalObjectiveProgress01);
};
