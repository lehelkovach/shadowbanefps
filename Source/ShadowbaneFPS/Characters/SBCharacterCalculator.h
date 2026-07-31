// Copyright shadowbanefps.
//
// shadowbanefps / Morloch creation calculator compressed for FPS pilot.
// Rules: 55 creation points, race cost, base 40 after “remove five from all”,
// then race+base class grants, auto-spend remaining into role-biased stats.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SBShadowbaneCreationDb.h"
#include "SBCharacterCalculator.generated.h"

USTRUCT(BlueprintType)
struct FSBCreationStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) int32 Strength = 40;
	UPROPERTY(BlueprintReadOnly) int32 Dexterity = 40;
	UPROPERTY(BlueprintReadOnly) int32 Constitution = 40;
	UPROPERTY(BlueprintReadOnly) int32 Intelligence = 40;
	UPROPERTY(BlueprintReadOnly) int32 Spirit = 40;
};

USTRUCT(BlueprintType)
struct FSBCreationFpsVitals
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) float MaxHealth = 100.f;
	UPROPERTY(BlueprintReadOnly) float MaxMana = 100.f;
	UPROPERTY(BlueprintReadOnly) float MaxStamina = 100.f;
	UPROPERTY(BlueprintReadOnly) float ManaRegen = 8.f;
	UPROPERTY(BlueprintReadOnly) float StaminaRegen = 14.f;
	UPROPERTY(BlueprintReadOnly) float MoveSpeed = 600.f;
	UPROPERTY(BlueprintReadOnly) float AttackDamage = 18.f;
	UPROPERTY(BlueprintReadOnly) bool bPreferMelee = true;
};

/** HUD-facing ability line for the selected discipline. */
USTRUCT(BlueprintType)
struct FSBCreationAbilityLine
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FString Name;
	UPROPERTY(BlueprintReadOnly) FString Summary;
};

USTRUCT(BlueprintType)
struct FSBCharacterBuildState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FString Race = TEXT("Human");
	UPROPERTY(BlueprintReadOnly) FString BaseClass = TEXT("Fighter");
	UPROPERTY(BlueprintReadOnly) FString Prestige = TEXT("Warrior");
	/** Pilot: one selected discipline (SB allows up to 3 pre-70). */
	UPROPERTY(BlueprintReadOnly) FString Discipline;
	UPROPERTY(BlueprintReadOnly) FSBCreationStats Stats;
	UPROPERTY(BlueprintReadOnly) FSBCreationFpsVitals Vitals;
	UPROPERTY(BlueprintReadOnly) int32 PointsRemaining = 55;
	UPROPERTY(BlueprintReadOnly) int32 RaceCost = 0;
	UPROPERTY(BlueprintReadOnly) bool bValid = false;
	UPROPERTY(BlueprintReadOnly) FString StatusLine;
	UPROPERTY(BlueprintReadOnly) TArray<FString> InnateAbilities;
	UPROPERTY(BlueprintReadOnly) TArray<FSBCreationAbilityLine> DisciplineAbilities;
};

UCLASS()
class SHADOWBANEFPS_API USBCharacterCalculator : public UObject
{
	GENERATED_BODY()

public:
	/** Recalculate stats + FPS vitals from race / base / prestige / discipline. */
	static void Recalculate(FSBCharacterBuildState& InOutBuild);

	/** Cycle helpers for the death-time builder UI. */
	static void CycleRace(FSBCharacterBuildState& InOutBuild, int32 Delta);
	static void CycleBaseClass(FSBCharacterBuildState& InOutBuild, int32 Delta);
	static void CyclePrestige(FSBCharacterBuildState& InOutBuild, int32 Delta);
	static void CycleDiscipline(FSBCharacterBuildState& InOutBuild, int32 Delta);

	/** Best pilot roster match for this shadowbanefps pick (by race + prestige/class). */
	static class USBCharacterArchetype* FindBestRosterMatch(
		const FSBCharacterBuildState& Build,
		const TArray<TObjectPtr<USBCharacterArchetype>>& Roster);
};
