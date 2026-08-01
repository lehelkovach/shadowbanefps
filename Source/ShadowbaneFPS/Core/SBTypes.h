// Copyright shadowbanefps.
//
// Shared enums / structs for the Conquest Siege pilot.
// Cross-reference: docs/game-design.md sections 5, 7, 8.

#pragma once

#include "CoreMinimal.h"
#include "SBTypes.generated.h"

/** Which side of the asymmetrical siege a player belongs to. See design doc §5. */
UENUM(BlueprintType)
enum class ESBTeam : uint8
{
	Unassigned	UMETA(DisplayName = "Unassigned"),
	Attackers	UMETA(DisplayName = "Attackers"),
	Defenders	UMETA(DisplayName = "Defenders")
};

/** Match ruleset. Siege = 5v5 conquest; FreeForAll = open dogfood deathmatch. */
UENUM(BlueprintType)
enum class ESBMatchMode : uint8
{
	Siege		UMETA(DisplayName = "Siege"),
	FreeForAll	UMETA(DisplayName = "Free For All")
};

/** High-level match phases used for pacing / telemetry. See design doc §8. */
UENUM(BlueprintType)
enum class ESBMatchPhase : uint8
{
	WaitingToStart	UMETA(DisplayName = "Waiting To Start"),
	Recon			UMETA(DisplayName = "Phase 1 - Reconnaissance & Outer Siege"),
	Breach			UMETA(DisplayName = "Phase 2 - Breach & Courtyard Conquest"),
	InnerAssault	UMETA(DisplayName = "Phase 3 - Inner Assault"),
	Overtime		UMETA(DisplayName = "Overtime"),
	Finished		UMETA(DisplayName = "Finished")
};

/**
 * Conquest ownership stage. Advancing a stage moves the attacker forward spawn
 * up and pushes the defender spawn inward. See design doc §7 (Conquest state).
 */
UENUM(BlueprintType)
enum class ESBConquestStage : uint8
{
	OuterSiege	UMETA(DisplayName = "Outer Siege"),
	Courtyard	UMETA(DisplayName = "Courtyard Held"),
	InnerKeep	UMETA(DisplayName = "Inner Keep")
};

/** Damage state of a destructible siege structure. See design doc §7. */
UENUM(BlueprintType)
enum class ESBStructureState : uint8
{
	Intact		UMETA(DisplayName = "Intact"),
	Damaged		UMETA(DisplayName = "Damaged"),
	Destroyed	UMETA(DisplayName = "Destroyed")
};

/** Result of a finished match. */
UENUM(BlueprintType)
enum class ESBMatchResult : uint8
{
	Undecided		UMETA(DisplayName = "Undecided"),
	AttackersWin	UMETA(DisplayName = "Attackers Win"),
	DefendersWin	UMETA(DisplayName = "Defenders Win")
};
