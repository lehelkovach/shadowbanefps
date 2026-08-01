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

	/**
	 * Server capture-zone tick (mirrors ASBCapturePoint):
	 * uncontested attackers advance, empty/defender-only decays, contested holds.
	 */
	UFUNCTION(BlueprintPure, Category = "Siege|Rules|Server")
	static float TickCaptureProgress(
		float CurrentProgressSeconds,
		float DeltaSeconds,
		int32 AttackersInZone,
		int32 DefendersInZone,
		float CaptureSeconds,
		float DecayPerSecond = 0.5f);

	UFUNCTION(BlueprintPure, Category = "Siege|Rules|Server")
	static bool IsCaptureComplete(float ProgressSeconds, float CaptureSeconds);

	/**
	 * Server final-objective tick (mirrors ASBConquestObjective):
	 * uncontested attackers advance, uncontested empty decays, contested holds.
	 */
	UFUNCTION(BlueprintPure, Category = "Siege|Rules|Server")
	static float TickObjectiveProgress(
		float CurrentProgressSeconds,
		float DeltaSeconds,
		int32 AttackersInZone,
		int32 DefendersInZone,
		float CompleteSeconds,
		float DecayRatePerSecond);

	UFUNCTION(BlueprintPure, Category = "Siege|Rules|Server")
	static bool IsObjectiveComplete(float ProgressSeconds, float CompleteSeconds);

	/** Normalized 0..1 progress helpers used by HUD + GameState. */
	UFUNCTION(BlueprintPure, Category = "Siege|Rules")
	static float NormalizeProgress(float ProgressSeconds, float TotalSeconds);

	/** Client HUD clock — "MM:SS" from remaining regulation seconds. */
	UFUNCTION(BlueprintPure, Category = "Siege|Rules|Client")
	static FString FormatMatchClock(float RemainingSeconds);

	/** Archetype switch is only legal while dead (design doc §9). */
	UFUNCTION(BlueprintPure, Category = "Siege|Rules|Client")
	static bool CanSelectArchetypeWhileDead(bool bAlive);

	/** Client may request respawn once the server countdown finishes. */
	UFUNCTION(BlueprintPure, Category = "Siege|Rules|Client")
	static bool CanRequestRespawn(bool bCanRespawn, float RespawnTimeRemaining);

	/** Client death overlay should show while dead (no pawn / not alive). */
	UFUNCTION(BlueprintPure, Category = "Siege|Rules|Client")
	static bool ShouldShowDeathOverlay(bool bAlive, bool bHasPawn);

	/** Friendly-fire gate used by server combat. */
	UFUNCTION(BlueprintPure, Category = "Siege|Rules|Server")
	static bool IsFriendlyFire(ESBTeam ShooterTeam, ESBTeam TargetTeam);

	/** FreeForAll matches never treat targets as friendly (everyone is hostile). */
	static bool IsFriendlyFire(ESBTeam ShooterTeam, ESBTeam TargetTeam, bool bFreeForAll);

	/** Reads replicated MatchMode from GameState when present. */
	UFUNCTION(BlueprintPure, Category = "Siege|Rules")
	static bool IsWorldFreeForAll(const UObject* WorldContextObject);

	/** True if same team and not FFA (heals / ally auras). Self is always an ally. */
	static bool AreAllies(ESBTeam A, ESBTeam B, bool bFreeForAll, bool bIsSelf);

	/**
	 * Split a bot pool into attackers/defenders for populate tests.
	 * Prefer filling toward 5v5; leftovers go to the smaller side (Attackers on tie).
	 */
	UFUNCTION(BlueprintPure, Category = "Siege|Rules|Bots")
	static void SplitBotsAcrossTeams(int32 TotalBots, int32 MaxPerTeam, int32& OutAttackers, int32& OutDefenders);
};
