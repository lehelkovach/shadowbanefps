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

	/** Fighter / Healer / Mage / Rogue — Morloch Four Paths. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shadowbane")
	FText BasePath;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shadowbane")
	FText Class;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shadowbane")
	FText Promotion;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shadowbane")
	FText Discipline;

	/** The pawn spawned for this archetype. Empty = use the shared ASBCharacter. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	TSoftClassPtr<APawn> PawnClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	FSBRoleProfile RoleProfile;

	/** Combat / traversal stats applied to the shared pawn at spawn. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float MaxHealth = 100.f;

	/** Race/class mana pool (Spirit/Int bias). HUD shows current/max as a bar. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float MaxMana = 100.f;

	/** Race/class stamina pool (Constitution bias). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float MaxStamina = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float ManaRegenPerSecond = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float StaminaRegenPerSecond = 14.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float MoveSpeed = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float AttackDamage = 18.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float AttackRange = 2500.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float AttackInterval = 0.35f;

	/** Multiplier when damaging ASBDestructibleStructure actors. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float StructureDamage = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float HealPerSecond = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float RepairPerSecond = 0.f;

	/** Short readable signature used by the intel / death-recap systems (§4). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Intel")
	FText ObservableSignature;

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
	int32 PerTeamDuplicateLimit = 1;

	/**
	 * True = close-range server sweep (sword/blade). False = hitscan (bow/bolt).
	 * Set from roster so Warrior / Assassin swing while Rangers shoot.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	bool bMeleeAttack = false;
};
