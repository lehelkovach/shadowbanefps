// Copyright shadowbanefps.
//
// A fully pre-built, immediately playable roster character.
// The pilot selects a *character*, not a loose bundle of skills. See design doc §3.
//
// Design intent: a curated roster of ~8-12 of these forms the selectable pool.
// The archetype's role profile (below) is what the lobby surfaces so a team can
// read its damage / healing / control / mobility / detection / siege coverage and gaps.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "SBCharacterArchetype.generated.h"

class APawn;

/**
 * Coarse 0-3 role weighting used only for lobby readability + telemetry archetype
 * bucketing (design doc §3, §12). It is NOT a balance stat block; actual capability
 * lives in the GameplayAbilities / pawn setup referenced by PawnClass.
 */
USTRUCT(BlueprintType)
struct FSBRoleProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0", ClampMax = "3"))
	uint8 Damage = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0", ClampMax = "3"))
	uint8 Healing = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0", ClampMax = "3"))
	uint8 Control = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0", ClampMax = "3"))
	uint8 Mobility = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0", ClampMax = "3"))
	uint8 Detection = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0", ClampMax = "3"))
	uint8 Siege = 0;
};

UCLASS(BlueprintType)
class SHADOWBANEFPS_API USBCharacterArchetype : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Stable id used for telemetry pick-rate tracking and duplicate limits. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FName ArchetypeId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FText DisplayName;

	/** Shadowbane-derived descriptors, shown to the owning team in the lobby (§3). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shadowbane")
	FText Race;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shadowbane")
	FText Class;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shadowbane")
	FText Promotion;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shadowbane")
	FText Discipline;

	/** The pawn spawned for this archetype (abilities, mesh, movement, etc.). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	TSoftClassPtr<APawn> PawnClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	FSBRoleProfile RoleProfile;

	/** Some archetypes only make sense on one side (e.g. defender emplacement crew). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	bool bAttackerEligible = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	bool bDefenderEligible = true;

	/**
	 * Heavy siege / emplacement archetypes may only deploy from specific spawns
	 * (design doc §9). Tag matched against spawn point tags by the GameMode.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	FGameplayTagContainer RequiredSpawnTags;

	/** Max copies of this archetype allowed per team (0 = unlimited). See §3, §9. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rules", meta = (ClampMin = "0"))
	int32 PerTeamDuplicateLimit = 0;
};
